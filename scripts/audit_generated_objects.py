#!/usr/bin/env python3
"""Audit generated object shape and link-smoke symbol coverage.

The detailed symbol inventory is private input.  Diagnostics intentionally use
only fixed role names, ordinals, counts, digests, and reason codes so a failed
local audit can be summarized without exposing target-derived names or paths.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Iterable, Sequence


IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
INVENTORY_KEYS = {
    "version",
    "baseline_body_symbols",
    "baseline_body_data_symbols",
    "normal_callable_symbols",
    "normal_wrapper_data_symbols",
    "patch_symbols",
    "patch_data_symbols",
    "support_lifecycle_symbols",
    "support_data_symbols",
    "runtime_bridge_symbols",
    "runtime_data_symbols",
    "link_smoke_entry_symbols",
    "coverage_symbol_count",
    "coverage_sha256",
    "section_count",
}
INVENTORY_KEYS_V2 = INVENTORY_KEYS | {
    "alternate_entry_thunk_symbols",
    "table_support_symbols",
    "section_address_support_symbols",
    "lookup_support_symbols",
    "lifecycle_support_symbols",
    "relocation_support_symbols",
}
LIST_KEYS = (
    "baseline_body_symbols",
    "baseline_body_data_symbols",
    "normal_callable_symbols",
    "normal_wrapper_data_symbols",
    "patch_symbols",
    "patch_data_symbols",
    "support_lifecycle_symbols",
    "support_data_symbols",
    "runtime_bridge_symbols",
    "runtime_data_symbols",
    "link_smoke_entry_symbols",
)
ROLE_PARTITION_KEYS = (
    "alternate_entry_thunk_symbols",
    "table_support_symbols",
    "section_address_support_symbols",
    "lookup_support_symbols",
    "lifecycle_support_symbols",
    "relocation_support_symbols",
)
OBJECT_ROLES = (
    "baseline-body",
    "normal-wrapper",
    "patch",
    "support-lifecycle",
)
AUDITED_ROLES = (*OBJECT_ROLES, "runtime-bridge", "link-smoke")
MAX_SECTION_COUNT = 4096
COMPILER_UNDEFINED_ALLOWLIST = {
    "_GLOBAL_OFFSET_TABLE_",
    "_RTC_CheckStackVars",
    "_RTC_InitBase",
    "_RTC_Shutdown",
    "__GSHandlerCheck",
    "__security_check_cookie",
    "__security_cookie",
    "__stack_chk_fail",
    "__stack_chk_guard",
    "memset",
}
# The MSVC-compatible UCRT fenv.h header defines this default environment as
# selectany data. Both cl and clang-cl therefore emit the same header-owned
# COFF datum in every object that includes recomp.h. It is excluded only when
# its exact SDK name and section metadata match the UCRT definition.
MSVC_UCRT_FENV_SYMBOL = "_Fenv1"
MSVC_SCALAR_LITERAL = re.compile(r"^__real@(?:[0-9A-Fa-f]{8}|[0-9A-Fa-f]{16})$")
MSVC_VECTOR_LITERAL = re.compile(r"^__xmm@[0-9A-Fa-f]{32}$")
# clang-cl gives C string literals MSVC-compatible external COMDAT names.  The
# encoded A-P length is the section size in bytes (A=0 through P=15), which
# lets the audit distinguish an exact compiler literal from a lookalike symbol
# without inspecting or reporting the private literal payload.
MSVC_STRING_LITERAL = re.compile(
    r"^\?\?_C@_[01](?P<length>[B-P][A-P]*)@[A-P]{1,8}@.+@$"
)
# ELF C++ frontends can emit these exact weak exception artifacts when a
# runtime bridge contains a catch block. They are compiler bookkeeping, not
# project-owned bridge definitions. Keep this allowance deliberately narrower
# than normal weak-symbol rejection: it applies only to the runtime bridge,
# only to the recorded ELF symbol type, and only to these exact spellings.
GNU_EXCEPTION_METADATA = {
    ("DW.ref.__gxx_personality_v0", "V"),
    ("__clang_call_terminate", "W"),
}
SECTION_AUXILIARY = re.compile(
    r"^Section length\s+([0-9A-Fa-f]+), #relocs\s+([0-9A-Fa-f]+),.*"
    r"selection\s+(\d+)\b.*$"
)
SECTION_HEADER = re.compile(r"^SECTION HEADER #([0-9A-Fa-f]+)$")
SECTION_NAME = re.compile(r"^(\S+) name$")
SECTION_RAW_SIZE = re.compile(r"^([0-9A-Fa-f]+) size of raw data$")
SECTION_RELOCATIONS = re.compile(r"^([0-9A-Fa-f]+) number of relocations$")
SECTION_FLAGS = re.compile(r"^([0-9A-Fa-f]+) flags$")
COFF_INITIALIZED_DATA = 0x00000040
COFF_COMDAT = 0x00001000
COFF_EXECUTE = 0x20000000
COFF_READ = 0x40000000
COFF_WRITE = 0x80000000


class AuditFailure(Exception):
    """Expected, public-safe audit failure."""


def _digest(symbols: Iterable[str]) -> str:
    normalized = "".join(f"{symbol}\n" for symbol in sorted(symbols))
    return hashlib.sha256(normalized.encode("utf-8")).hexdigest()


def _canonical_bytes(value: object) -> bytes:
    return json.dumps(
        value,
        sort_keys=True,
        separators=(",", ":"),
        ensure_ascii=True,
        allow_nan=False,
    ).encode("utf-8")


def _write_result(path: Path | None, value: object, role: str) -> None:
    if path is None:
        return
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        if path.exists() or path.is_symlink():
            path.unlink()
        path.write_bytes(_canonical_bytes(value))
    except OSError as error:
        raise AuditFailure(f"{role}: result-output-failed") from error


def _load_lines(path: Path, role: str) -> list[Path]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError) as error:
        raise AuditFailure(f"{role}: object-list-unreadable") from error
    objects = [Path(line) for line in lines if line]
    if len(objects) != len(set(objects)):
        raise AuditFailure(f"{role}: duplicate-object-list-entry")
    for index, object_path in enumerate(objects, start=1):
        if not object_path.is_file():
            raise AuditFailure(f"{role}: object-{index}: missing-object")
    return objects


def _load_inventory(path: Path) -> dict[str, object]:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise AuditFailure("inventory: unreadable-or-invalid-json") from error
    if not isinstance(document, dict) or set(document) not in {
        frozenset(INVENTORY_KEYS),
        frozenset(INVENTORY_KEYS_V2),
    }:
        raise AuditFailure("inventory: invalid-member-set")
    if type(document.get("version")) is not int or document["version"] not in {1, 2}:
        raise AuditFailure("inventory: unsupported-version")
    expected_keys = INVENTORY_KEYS_V2 if document["version"] == 2 else INVENTORY_KEYS
    if set(document) != expected_keys:
        raise AuditFailure("inventory: version-member-mismatch")
    list_keys = (*LIST_KEYS, *ROLE_PARTITION_KEYS) if document["version"] == 2 else LIST_KEYS
    for key in list_keys:
        values = document.get(key)
        if not isinstance(values, list) or any(type(value) is not str for value in values):
            raise AuditFailure(f"inventory: {key}: invalid-list")
        if values != sorted(values) or len(values) != len(set(values)):
            raise AuditFailure(f"inventory: {key}: noncanonical-list")
        if any(not IDENTIFIER.fullmatch(value) for value in values):
            raise AuditFailure(f"inventory: {key}: invalid-identifier")

    count = document.get("coverage_symbol_count")
    digest = document.get("coverage_sha256")
    if type(count) is not int or count < 0:
        raise AuditFailure("inventory: invalid-coverage-count")
    if not isinstance(digest, str) or not re.fullmatch(r"[0-9a-f]{64}", digest):
        raise AuditFailure("inventory: invalid-coverage-digest")
    section_count = document.get("section_count")
    if type(section_count) is not int or not 0 < section_count <= MAX_SECTION_COUNT:
        raise AuditFailure("inventory: section-count-exceeds-runtime-capacity")
    if document["version"] == 2:
        support = set(document["support_lifecycle_symbols"])
        alternate = set(document["alternate_entry_thunk_symbols"])
        tables = set(document["table_support_symbols"])
        table_roles = [
            set(document[key])
            for key in (
                "section_address_support_symbols",
                "lookup_support_symbols",
                "lifecycle_support_symbols",
                "relocation_support_symbols",
            )
        ]
        if (
            alternate & tables
            or support != alternate | tables
            or any(
                table_roles[left] & table_roles[right]
                for left in range(len(table_roles))
                for right in range(left + 1, len(table_roles))
            )
            or tables != set().union(*table_roles)
            or tuple(map(len, table_roles)) != (
                3,
                3 if "jfg_generated_lookup_entry_offset" in table_roles[1] else 2,
                1,
                5,
            )
        ):
            raise AuditFailure("inventory: support-role-partition-mismatch")
    return document


def _run_tool(command: Sequence[str], role: str) -> str:
    try:
        result = subprocess.run(
            command,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except OSError as error:
        raise AuditFailure(f"{role}: symbol-tool-unavailable") from error
    if result.returncode != 0:
        raise AuditFailure(f"{role}: symbol-tool-failed")
    return result.stdout


def _path_key(path: str | Path) -> str:
    return os.path.normcase(os.path.abspath(os.fspath(path))).replace("\\", "/")


def _is_gnu_exception_metadata(
    symbol: str,
    symbol_type: str,
    role: str,
) -> bool:
    return (
        role == "runtime-bridge"
        and (symbol, symbol_type) in GNU_EXCEPTION_METADATA
    )


def _nm_symbols(tool: str, objects: Sequence[Path], role: str) -> tuple[list[set[str]], list[set[str]], list[set[str]], list[set[str]], list[set[str]]]:
    function_definitions = [set() for _ in objects]
    data_definitions = [set() for _ in objects]
    undefined = [set() for _ in objects]
    weak_function_definitions = [set() for _ in objects]
    weak_data_definitions = [set() for _ in objects]
    indices = {_path_key(path): index for index, path in enumerate(objects)}

    for chunk_start in range(0, len(objects), 32):
        chunk = objects[chunk_start : chunk_start + 32]
        output = _run_tool(
            [tool, "-A", "--format=posix", "--extern-only", *map(str, chunk)],
            role,
        )
        for line in output.splitlines():
            if ": " not in line:
                continue
            object_text, symbol_text = line.rsplit(": ", 1)
            index = indices.get(_path_key(object_text))
            fields = symbol_text.split()
            if index is None or len(fields) < 2:
                continue
            symbol, symbol_type = fields[0], fields[1]
            if symbol_type.upper() == "U":
                undefined[index].add(symbol)
            elif _is_gnu_exception_metadata(
                symbol, symbol_type, role
            ):
                continue
            elif symbol_type.upper() in {"T", "W"}:
                function_definitions[index].add(symbol)
                if symbol_type.upper() == "W":
                    weak_function_definitions[index].add(symbol)
            elif symbol_type.upper() in {"B", "C", "D", "G", "R", "S", "V"}:
                data_definitions[index].add(symbol)
                if symbol_type.upper() == "V":
                    weak_data_definitions[index].add(symbol)
    return (
        function_definitions,
        data_definitions,
        undefined,
        weak_function_definitions,
        weak_data_definitions,
    )


def _is_msvc_compiler_owned_data(
    symbol: str,
    role: str,
    compiler_frontend_variant: str,
    storage: str,
    section_name: str,
    section_length: int,
    relocation_count: int,
    selection: int,
    characteristics: int,
) -> bool:
    required_characteristics = COFF_INITIALIZED_DATA | COFF_COMDAT | COFF_READ
    forbidden_characteristics = COFF_EXECUTE | COFF_WRITE
    if (
        compiler_frontend_variant != "MSVC"
        or role not in AUDITED_ROLES
        or storage != "External"
        or section_name != ".rdata"
        or relocation_count != 0
        or selection != 2
        or characteristics & required_characteristics != required_characteristics
        or characteristics & forbidden_characteristics != 0
    ):
        return False
    if symbol == MSVC_UCRT_FENV_SYMBOL:
        return section_length == 8
    scalar = MSVC_SCALAR_LITERAL.fullmatch(symbol)
    if scalar is not None:
        encoded_width = len(symbol.rsplit("@", 1)[1])
        return section_length == encoded_width // 2
    if MSVC_VECTOR_LITERAL.fullmatch(symbol) is not None:
        return section_length == 16
    string_literal = MSVC_STRING_LITERAL.fullmatch(symbol)
    if string_literal is not None:
        encoded_length = string_literal.group("length")
        decoded_length = 0
        for digit in encoded_length:
            decoded_length = (decoded_length << 4) | (ord(digit) - ord("A"))
        return section_length == decoded_length
    return False


def _parse_dumpbin_section_auxiliary(
    line: str,
) -> tuple[int, int, int] | None:
    match = SECTION_AUXILIARY.fullmatch(line.strip())
    if match is None:
        return None
    # dumpbin prints section length and relocation count in hexadecimal, even
    # when the fields contain digits only. COMDAT selection is decimal.
    return (
        int(match.group(1), 16),
        int(match.group(2), 16),
        int(match.group(3), 10),
    )


def _parse_dumpbin_section_headers(
    output: str,
    indices: dict[str, int],
) -> dict[tuple[int, str], tuple[str, int, int, int]]:
    records: dict[tuple[int, str], dict[str, str]] = {}
    current_index: int | None = None
    current_key: tuple[int, str] | None = None
    for line in output.splitlines():
        stripped = line.strip()
        if stripped.startswith("Dump of file "):
            current_index = indices.get(_path_key(stripped[len("Dump of file ") :]))
            current_key = None
            continue
        header = SECTION_HEADER.fullmatch(stripped)
        if header is not None:
            current_key = (
                (current_index, f"SECT{header.group(1).upper()}")
                if current_index is not None
                else None
            )
            if current_key is not None:
                records[current_key] = {}
            continue
        if current_key is None:
            continue
        record = records[current_key]
        for key, pattern in (
            ("name", SECTION_NAME),
            ("size", SECTION_RAW_SIZE),
            ("relocations", SECTION_RELOCATIONS),
            ("flags", SECTION_FLAGS),
        ):
            match = pattern.fullmatch(stripped)
            if match is not None:
                record[key] = match.group(1)
                break

    parsed: dict[tuple[int, str], tuple[str, int, int, int]] = {}
    for key, record in records.items():
        if set(record) == {"name", "size", "relocations", "flags"}:
            parsed[key] = (
                record["name"],
                int(record["size"], 16),
                int(record["relocations"], 16),
                int(record["flags"], 16),
            )
    return parsed


def _dumpbin_symbols(
    tool: str,
    objects: Sequence[Path],
    role: str,
    compiler_frontend_variant: str,
) -> tuple[list[set[str]], list[set[str]], list[set[str]], list[set[str]], list[set[str]]]:
    function_definitions = [set() for _ in objects]
    data_definitions = [set() for _ in objects]
    undefined = [set() for _ in objects]
    weak_function_definitions = [set() for _ in objects]
    weak_data_definitions = [set() for _ in objects]
    indices = {_path_key(path): index for index, path in enumerate(objects)}

    for chunk_start in range(0, len(objects), 32):
        chunk = objects[chunk_start : chunk_start + 32]
        output = _run_tool(
            [tool, "/nologo", "/headers", "/symbols", *map(str, chunk)],
            role,
        )
        section_headers = _parse_dumpbin_section_headers(output, indices)
        current_index: int | None = None
        section_metadata: dict[tuple[int, str], tuple[str, int, int, int]] = {}
        pending_section: tuple[int, str, str] | None = None
        ignored_header_data = [set() for _ in objects]
        for line in output.splitlines():
            stripped = line.strip()
            if stripped.startswith("Dump of file "):
                current_index = indices.get(_path_key(stripped[len("Dump of file ") :]))
                pending_section = None
                continue
            if current_index is not None and pending_section is not None:
                auxiliary = _parse_dumpbin_section_auxiliary(stripped)
                if auxiliary is not None:
                    pending_index, pending_tag, pending_name = pending_section
                    section_metadata[(pending_index, pending_tag)] = (
                        pending_name,
                        *auxiliary,
                    )
                pending_section = None
            if current_index is None or " | " not in line:
                continue
            left, symbol = line.rsplit(" | ", 1)
            fields = left.split()
            if len(fields) < 4:
                continue
            section = fields[2]
            storage = fields[-1]
            normalized_symbol = symbol.strip()
            # dumpbin appends a human-readable parenthetical to MSVC string
            # literal symbols. The COFF symbol itself is the first token.
            coff_symbol = normalized_symbol.split(" ", 1)[0]
            if (
                section.startswith("SECT")
                and storage == "Static"
                and normalized_symbol.startswith(".")
            ):
                pending_section = (current_index, section, normalized_symbol)
            if section == "UNDEF" and storage in {"External", "WeakExternal"}:
                undefined[current_index].add(normalized_symbol)
            elif section.startswith("SECT") and storage in {"External", "WeakExternal"}:
                if "()" in fields:
                    function_definitions[current_index].add(normalized_symbol)
                    if storage == "WeakExternal":
                        weak_function_definitions[current_index].add(normalized_symbol)
                else:
                    metadata = section_metadata.get((current_index, section))
                    header_metadata = section_headers.get((current_index, section))
                    if (
                        metadata is not None
                        and header_metadata is not None
                        and header_metadata[:3] == metadata[:3]
                        and _is_msvc_compiler_owned_data(
                            coff_symbol,
                            role,
                            compiler_frontend_variant,
                            storage,
                            *metadata,
                            header_metadata[3],
                        )
                    ):
                        if coff_symbol in ignored_header_data[current_index]:
                            raise AuditFailure(
                                f"{role}: object-{current_index + 1}: duplicate-compiler-header-data"
                            )
                        ignored_header_data[current_index].add(coff_symbol)
                        continue
                    data_definitions[current_index].add(normalized_symbol)
                    if storage == "WeakExternal":
                        weak_data_definitions[current_index].add(normalized_symbol)
    return (
        function_definitions,
        data_definitions,
        undefined,
        weak_function_definitions,
        weak_data_definitions,
    )


def _read_symbols(
    tool_kind: str,
    tool: str,
    objects: Sequence[Path],
    role: str,
    compiler_frontend_variant: str,
) -> tuple[list[set[str]], list[set[str]], list[set[str]], list[set[str]], list[set[str]]]:
    if not objects:
        return [], [], [], [], []
    if tool_kind == "nm":
        return _nm_symbols(tool, objects, role)
    return _dumpbin_symbols(
        tool,
        objects,
        role,
        compiler_frontend_variant,
    )


def _audit_one_function_objects(
    role: str,
    expected: Sequence[str],
    expected_data: Sequence[str],
    definitions: Sequence[set[str]],
    data_definitions: Sequence[set[str]],
    weak_definitions: Sequence[set[str]],
    weak_data_definitions: Sequence[set[str]],
) -> None:
    if len(definitions) != len(expected):
        raise AuditFailure(
            f"{role}: object-count={len(definitions)} expected-symbol-count={len(expected)}"
        )
    flattened: list[str] = []
    flattened_data: list[str] = []
    for index, (object_definitions, object_data, object_weak, object_weak_data) in enumerate(
        zip(definitions, data_definitions, weak_definitions, weak_data_definitions),
        start=1,
    ):
        if object_weak:
            raise AuditFailure(f"{role}: object-{index}: weak-external-function")
        if object_weak_data:
            raise AuditFailure(f"{role}: object-{index}: weak-external-data")
        if len(object_definitions) != 1:
            raise AuditFailure(
                f"{role}: object-{index}: external-function-count={len(object_definitions)} expected=1"
            )
        flattened.extend(object_definitions)
        flattened_data.extend(object_data)
    if len(flattened) != len(set(flattened)):
        raise AuditFailure(f"{role}: duplicate-external-function")
    if sorted(flattened) != sorted(expected):
        raise AuditFailure(
            f"{role}: inventory-mismatch actual-count={len(flattened)} "
            f"actual-digest={_digest(flattened)}"
        )
    if len(flattened_data) != len(set(flattened_data)):
        raise AuditFailure(f"{role}: duplicate-external-data")
    if sorted(flattened_data) != sorted(expected_data):
        raise AuditFailure(
            f"{role}: data-inventory-mismatch actual-count={len(flattened_data)} "
            f"actual-digest={_digest(flattened_data)}"
        )


def audit(args: argparse.Namespace) -> None:
    inventory = _load_inventory(args.inventory)
    object_lists = {
        "baseline-body": _load_lines(args.baseline_body_objects, "baseline-body"),
        "normal-wrapper": _load_lines(args.normal_wrapper_objects, "normal-wrapper"),
        "patch": _load_lines(args.patch_objects, "patch"),
        "support-lifecycle": _load_lines(args.support_objects, "support-lifecycle"),
        "runtime-bridge": _load_lines(args.runtime_objects, "runtime-bridge"),
        "link-smoke": _load_lines(args.link_smoke_objects, "link-smoke"),
    }
    symbols = {
        role: _read_symbols(
            args.tool_kind,
            args.tool,
            objects,
            role,
            args.compiler_frontend_variant,
        )
        for role, objects in object_lists.items()
    }

    expected_by_role = {
        "baseline-body": (
            inventory["baseline_body_symbols"],
            inventory["baseline_body_data_symbols"],
        ),
        "normal-wrapper": (
            inventory["normal_callable_symbols"],
            inventory["normal_wrapper_data_symbols"],
        ),
        "patch": (inventory["patch_symbols"], inventory["patch_data_symbols"]),
        "support-lifecycle": (
            inventory["support_lifecycle_symbols"],
            inventory["support_data_symbols"],
        ),
    }
    normal_symbols = inventory["normal_callable_symbols"]
    body_symbols = inventory["baseline_body_symbols"]
    patch_symbols = inventory["patch_symbols"]
    assert isinstance(normal_symbols, list)
    assert isinstance(body_symbols, list)
    assert isinstance(patch_symbols, list)
    if sorted(f"{symbol}_recomp" for symbol in normal_symbols) != body_symbols:
        raise AuditFailure("inventory: normal-to-body-alias-mismatch")
    if not set(patch_symbols).issubset(normal_symbols):
        raise AuditFailure("inventory: patch-not-in-normal-callable-set")

    runtime_definitions, runtime_data_definitions, _, runtime_weak, runtime_data_weak = symbols["runtime-bridge"]
    if any(runtime_weak) or any(runtime_data_weak):
        raise AuditFailure("runtime-bridge: weak-external-function")
    actual_runtime = sorted(symbol for values in runtime_definitions for symbol in values)
    expected_runtime = inventory["runtime_bridge_symbols"]
    assert isinstance(expected_runtime, list)
    if actual_runtime != expected_runtime:
        raise AuditFailure(
            f"runtime-bridge: inventory-mismatch actual-count={len(actual_runtime)} "
            f"actual-digest={_digest(actual_runtime)}"
        )

    expected_runtime_data = inventory["runtime_data_symbols"]
    assert isinstance(expected_runtime_data, list)
    actual_runtime_data = sorted(
        symbol for values in runtime_data_definitions for symbol in values
    )
    if actual_runtime_data != expected_runtime_data:
        raise AuditFailure(
            f"runtime-data: inventory-mismatch actual-count={len(actual_runtime_data)} "
            f"actual-digest={_digest(actual_runtime_data)}"
        )
    forbidden_runtime_data = set(expected_runtime_data)
    for role in OBJECT_ROLES:
        role_functions, role_data, _, _, _ = symbols[role]
        generated_definitions = set().union(*role_functions, *role_data)
        forbidden_count = len(generated_definitions & forbidden_runtime_data)
        if forbidden_count:
            raise AuditFailure(
                f"{role}: forbidden-runtime-data-owner-count={forbidden_count}"
            )

    for role in OBJECT_ROLES:
        definitions, data_definitions, _, weak, weak_data = symbols[role]
        expected, expected_data = expected_by_role[role]
        assert isinstance(expected, list) and isinstance(expected_data, list)
        _audit_one_function_objects(
            role,
            expected,
            expected_data,
            definitions,
            data_definitions,
            weak,
            weak_data,
        )

    smoke_definitions, _, smoke_undefined, smoke_weak, _ = symbols["link-smoke"]
    if any(smoke_weak):
        raise AuditFailure("link-smoke: weak-external-function")
    actual_smoke_definitions = sorted(
        symbol for values in smoke_definitions for symbol in values
    )
    expected_entries = inventory["link_smoke_entry_symbols"]
    assert isinstance(expected_entries, list)
    if actual_smoke_definitions != expected_entries:
        raise AuditFailure(
            f"link-smoke: entry-inventory-mismatch actual-count={len(actual_smoke_definitions)} "
            f"actual-digest={_digest(actual_smoke_definitions)}"
        )

    support_symbols = inventory["support_lifecycle_symbols"]
    runtime_data_symbols = inventory["runtime_data_symbols"]
    generated_data_symbols = (
        inventory["baseline_body_data_symbols"]
        + inventory["normal_wrapper_data_symbols"]
        + inventory["patch_data_symbols"]
        + inventory["support_data_symbols"]
    )
    assert isinstance(support_symbols, list)
    assert isinstance(runtime_data_symbols, list)
    assert all(isinstance(symbol, str) for symbol in generated_data_symbols)
    if len(generated_data_symbols) != len(set(generated_data_symbols)):
        raise AuditFailure("inventory: duplicate-generated-data-owner")
    function_symbols = (
        set(body_symbols)
        | set(normal_symbols)
        | set(support_symbols)
        | set(expected_runtime)
    )
    if set(generated_data_symbols) & (function_symbols | set(runtime_data_symbols)):
        raise AuditFailure("inventory: generated-data-category-overlap")
    coverage_symbols = sorted(
        function_symbols
        | set(generated_data_symbols)
        | set(runtime_data_symbols)
    )
    if inventory["coverage_symbol_count"] != len(coverage_symbols):
        raise AuditFailure("inventory: coverage-count-mismatch")
    if inventory["coverage_sha256"] != _digest(coverage_symbols):
        raise AuditFailure("inventory: coverage-digest-mismatch")
    referenced_symbols = set(symbol for values in smoke_undefined for symbol in values)
    missing_count = len(set(coverage_symbols) - referenced_symbols)
    if missing_count:
        raise AuditFailure(
            f"link-smoke: incomplete-coverage missing-count={missing_count} "
            f"expected-count={len(coverage_symbols)}"
        )
    unexpected_symbols = (
        referenced_symbols - set(coverage_symbols) - COMPILER_UNDEFINED_ALLOWLIST
    )
    if unexpected_symbols:
        raise AuditFailure(
            f"link-smoke: unexpected-reference-count={len(unexpected_symbols)}"
        )

    host_function_records = [
        {
            "symbol_sha256": hashlib.sha256(symbol.encode("utf-8")).hexdigest(),
            "owner_role": "minimal-runtime",
        }
        for symbol in actual_runtime
    ]
    host_function_records.sort(key=lambda row: row["symbol_sha256"])
    host_data_records = [
        {
            "symbol_sha256": hashlib.sha256(symbol.encode("utf-8")).hexdigest(),
            "owner_role": "minimal-runtime",
        }
        for symbol in actual_runtime_data
    ]
    host_data_records.sort(key=lambda row: row["symbol_sha256"])
    if not host_function_records or not host_data_records:
        raise AuditFailure("runtime-bridge: empty-owner-inventory")
    _write_result(
        args.host_function_inventory_output,
        {
            "schema_version": 1,
            "kind": "jfg-phase4-host-function-inventory",
            "record_count": len(host_function_records),
            "records": host_function_records,
        },
        "host-function-inventory",
    )
    _write_result(
        args.host_data_inventory_output,
        {
            "schema_version": 1,
            "kind": "jfg-phase4-host-data-inventory",
            "record_count": len(host_data_records),
            "records": host_data_records,
        },
        "host-data-inventory",
    )
    if inventory["version"] == 2:
        role_counts = {
            "baseline-body": len(object_lists["baseline-body"]),
            "normal-wrapper": len(object_lists["normal-wrapper"]),
            "patch": len(object_lists["patch"]),
            "alternate-entry-thunk": len(inventory["alternate_entry_thunk_symbols"]),
            "section-address-support": len(
                inventory["section_address_support_symbols"]
            ),
            "lookup-support": len(inventory["lookup_support_symbols"]),
            "lifecycle-support": len(inventory["lifecycle_support_symbols"]),
            "relocation-support": len(inventory["relocation_support_symbols"]),
            "runtime-function": len(actual_runtime),
            "runtime-data": len(actual_runtime_data),
        }
        if (
            role_counts["alternate-entry-thunk"]
            + role_counts["section-address-support"]
            + role_counts["lookup-support"]
            + role_counts["lifecycle-support"]
            + role_counts["relocation-support"]
            != len(object_lists["support-lifecycle"])
        ):
            raise AuditFailure("support-lifecycle: role-count-mismatch")
        _write_result(
            args.object_role_inventory_output,
            {
                "schema_version": 1,
                "kind": "jfg-phase4-generated-object-role-inventory",
                "role_counts": role_counts,
                "one_external_function_per_generated_object": True,
                "object_symbol_ownership_verified": True,
            },
            "object-role-inventory",
        )


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tool-kind", choices=("nm", "dumpbin"), required=True)
    parser.add_argument("--tool", required=True)
    parser.add_argument("--compiler-frontend-variant", required=True)
    parser.add_argument("--inventory", type=Path, required=True)
    parser.add_argument("--baseline-body-objects", type=Path, required=True)
    parser.add_argument("--normal-wrapper-objects", type=Path, required=True)
    parser.add_argument("--patch-objects", type=Path, required=True)
    parser.add_argument("--support-objects", type=Path, required=True)
    parser.add_argument("--runtime-objects", type=Path, required=True)
    parser.add_argument("--link-smoke-objects", type=Path, required=True)
    parser.add_argument("--host-function-inventory-output", type=Path)
    parser.add_argument("--host-data-inventory-output", type=Path)
    parser.add_argument("--object-role-inventory-output", type=Path)
    return parser.parse_args(argv)


def main(argv: Sequence[str] | None = None) -> int:
    try:
        audit(parse_args(sys.argv[1:] if argv is None else argv))
    except AuditFailure as error:
        print(f"generated-object-audit: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
