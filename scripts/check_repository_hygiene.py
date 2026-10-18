#!/usr/bin/env python3
"""Reject private data, ROM-like binaries, secrets, and oversized files."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path, PurePosixPath

MAX_TRACKED_BYTES = 1_048_576
MAX_HISTORY_SCAN_BYTES = 2_097_152
# Exact reviewed OFL font bodies only. No suffix-based binary exemption.
# Provenance and license texts: launcher/ui/fonts/manifest.json and *-OFL.txt.
APPROVED_UI_FONT_SHA256 = frozenset({
    "95aa02c7c43096e0dd44d787ba6216864a67157e402adab59b35572e0c1577ea",
    "86577cb32f8abe3673db53ca0f4221e6856751a4f6730c867e00f720f8bb1fc5",
    "65fbf76d95651697275e19db4d717c0e95a789ddd3476478b05292104db278a0",
})
# Exact reviewed custom map screenshot and project icon; never all PNG/ICO files.
# The owner approved publishing the custom map screenshot. Inventory art is
# extracted locally and is not part of this allowlist.
APPROVED_UI_IMAGE_SHA256 = frozenset({
    "c1396dc5a1db5756fdf5ae956f836b40496e4881f1e4507752cfa55c4bbaec9d",
    "e984be1b6a23919e2d73f2d5b0c38f3b52f9999521da824717f094e9389f8163",
})
# Exact reviewed artwork and historical false positives. Changed bodies require
# a fresh review; no path, extension, or credential exemption is granted.
REVIEWED_BODIES = json.loads((Path(__file__).with_name("reviewed_hygiene_bodies.json")).read_text(encoding="utf-8"))["bodies"]
REVIEWED_BY_SHA256 = {entry["sha256"]: entry for entry in REVIEWED_BODIES}
REVIEWED_LARGE_GIT_BLOBS = {entry["git_blob"] for entry in REVIEWED_BODIES if "size" in entry["exceptions"]}

# These are documented inventory bit identifiers, not embedded image bytes.
INVENTORY_IDS_DECLARATION = re.compile(
    r"\b(?:int ids\[\]|int\[\] itemIds)\s*=\s*\{"
    + r"\s*,\s*".join(map(str, (0, 1, 2, 3, 9, 16, 17, *range(20, 28))))
    + r"\s*\};"
)
# Preserve the exact historical Quarry findings body after removing its local
# checkout path from the current document. Only historical path reporting is
# exempted; credential, generated-data and other checks still apply.
HISTORICAL_CHECKOUT_PATH_SHA256 = frozenset({
    "e9941c31172d9a99e1b56419186b47698939d89e5372f5808610a400a1c9a455",
})
ALLOWED_GIT_IDENTITY_NAME = "TK22-26"
ALLOWED_GIT_IDENTITY_EMAIL = "254768757+TK22-26@users.noreply.github.com"
# Keep the original public snapshot valid without rewriting its history.
ALLOWED_GIT_IDENTITIES = frozenset({
    (ALLOWED_GIT_IDENTITY_NAME, ALLOWED_GIT_IDENTITY_EMAIL),
    ("JFG Recomp Maintainer", "jfg-recomp-local@users.noreply.github.com"),
})
# Contributors retain their own chosen identities. Reserve the maintainer
# pairs above so this does not relax the owner's personal-email policy.
CONTRIBUTOR_EMAIL_PATTERN = re.compile(r"[^@\s<>]+@[^@\s<>]+")


def approved_git_identity(name: str, email: str) -> bool:
    if (name, email) in ALLOWED_GIT_IDENTITIES:
        return True
    if any(name.casefold() == known_name.casefold() or email.casefold() == known_email.casefold()
           for known_name, known_email in ALLOWED_GIT_IDENTITIES):
        return False
    return CONTRIBUTOR_EMAIL_PATTERN.fullmatch(email) is not None


FORBIDDEN_PREFIXES = (
    "roms/",
    "tools/",
    "private-data/",
    "extracted/",
    "generated/",
    "src/generated/",
    "corpus/private/",
    "assets/private/",
    "artifacts/private/",
    "captures/private/",
)

FORBIDDEN_BASENAMES = frozenset({".gitmodules"})

FORBIDDEN_SUFFIXES = {
    ".z64",
    ".n64",
    ".v64",
    ".rom",
    ".elf",
    ".bin",
    ".sav",
    ".eep",
    ".sra",
    ".fla",
    ".pfs",
    ".state",
    ".rdram",
    ".pcm",
    ".wav",
    ".log",
    ".dmp",
    ".core",
    ".dump",
    ".trace",
    ".map",
    ".sym",
    ".ips",
    ".bps",
    ".xdelta",
    ".vcdiff",
    ".zip",
    ".7z",
    ".rar",
    ".tar",
    ".tgz",
    ".gz",
    ".bz2",
    ".xz",
    ".lz4",
    ".zst",
    ".cab",
    ".iso",
}

KNOWN_PROTECTED_SHA1 = frozenset(
    {
        # Supported Jet Force Gemini (US) retail ROM. Hashes are public-safe;
        # matching bodies are not.
        "493ced9008dbe932d6e91179b68e8630cf23a023",
    }
)

TEXT_SECRET_PATTERNS = (
    re.compile("gh" + r"[op]_[A-Za-z0-9_]{20,}"),
    re.compile("github" + r"_pat_[A-Za-z0-9_]{20,}"),
    re.compile(r"AKIA[0-9A-Z]{16}"),
    # Match a key prefix, not the suffix of words such as "task" or "mask"
    # in descriptive experiment identifiers. Quoted and assigned keys still match.
    re.compile(r"(?<![A-Za-z0-9])" + "sk" + r"-(?:proj-)?[A-Za-z0-9_-]{20,}"),
    re.compile("xox" + r"[baprs]-[A-Za-z0-9-]{10,}"),
    re.compile("AIza" + r"[0-9A-Za-z_-]{30,}"),
    re.compile(
        "eyJ"
        + r"[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}"
    ),
    re.compile("BEGIN " + r"(?:RSA |EC |OPENSSH )?PRIVATE KEY"),
)

LOCAL_PATH_PATTERNS = (
    re.compile(r"[A-Za-z]:[\\/]{1,2}Users[\\/]{1,2}[^\\/\s]+", re.IGNORECASE),
    re.compile(r"/mnt/[a-z]/" + r"Users/[^/\s]+/", re.IGNORECASE),
    re.compile(r"/(?:home|" + r"Users)/[^/\s]+/"),
)

# This exact public-safe placeholder appeared in the initial scanner test blob.
# Keeping the exemption narrow lets history scans recognize JSON-escaped real
# profile paths without making that synthetic canary poison the repository.
SAFE_LOCAL_PATH_MARKERS = (
    b"C:"
    + (b"\\" * 2)
    + b"Users"
    + (b"\\" * 2)
    + b"Example"
    + (b"\\" * 2)
    + b"file.txt",
)

N64_MAGICS = (
    bytes.fromhex("80371240"),
    bytes.fromhex("37804012"),
    bytes.fromhex("40123780"),
)

ARCHIVE_SIGNATURES = (
    (
        "ZIP",
        (
            bytes.fromhex("504b0304"),
            bytes.fromhex("504b0506"),
            bytes.fromhex("504b0708"),
        ),
    ),
    ("7-Zip", (bytes.fromhex("377abcaf271c"),)),
    (
        "RAR",
        (bytes.fromhex("526172211a0700"), bytes.fromhex("526172211a070100")),
    ),
    ("gzip", (bytes.fromhex("1f8b"),)),
    ("bzip2", (b"BZh",)),
    ("XZ", (bytes.fromhex("fd377a585a00"),)),
    ("LZ4", (bytes.fromhex("04224d18"),)),
    ("Zstandard", (bytes.fromhex("28b52ffd"),)),
    ("CAB", (b"MSCF",)),
)

PRIVATE_CORPUS_MARKERS = (
    b"JFG_" + b"PRIVATE_CORPUS_V1",
    b"JFG_" + b"PRIVATE_REGENERABLE_BODY_V1",
)

GENERATED_TEXT_PATTERNS = (
    re.compile("RECOMP" + r"_FUNC\s+void\s+[A-Za-z_][A-Za-z0-9_]*\s*\("),
)

LONG_OPAQUE_BASE64_PATTERN = re.compile(
    r"(?<![A-Za-z0-9+/])[A-Za-z0-9+/]{256,}={0,2}(?![A-Za-z0-9+/=])"
)
CHUNKED_OPAQUE_PAYLOAD_PATTERN = re.compile(
    r"(?<![A-Za-z0-9+/])(?:[A-Za-z0-9+/]{48,}={0,2}[\t ,:;'\"-]+)+"
    r"[A-Za-z0-9+/]{48,}={0,2}(?![A-Za-z0-9+/=])"
)
REPEATED_OPAQUE_TOKEN_PATTERN = re.compile(
    r"(?<![A-Za-z0-9+/])(?:[A-Za-z0-9+/]{16,}={0,2}[\t ,:;'\"-]+){3,}"
    r"[A-Za-z0-9+/]{16,}={0,2}(?![A-Za-z0-9+/=])"
)
REPEATED_IDENTICAL_OPAQUE_TOKEN_PATTERN = re.compile(
    r"(?<![A-Za-z0-9+/])([A-Za-z0-9+/]{8,})(?:[\t ,:;'\"-]+\1){3,}"
    r"(?![A-Za-z0-9+/=])"
)
SPACED_HEX_PAYLOAD_PATTERN = re.compile(
    # A separator is whitespace or one punctuation character with whitespace.
    # Treating comma-minus as a separator misreads signed decimal arrays as hex.
    r"(?<![0-9A-Fa-f+-])(?:[0-9A-Fa-f]{2}(?:\s+|\s*[,:-]\s*)){7,}"
    r"[0-9A-Fa-f]{2}(?![0-9A-Fa-f])"
)
READELF_SYMBOL_LINE_PATTERN = re.compile(
    r"^\s*\d+:\s+[0-9A-Fa-f]{8,16}\s+(?:0x[0-9A-Fa-f]+|\d+)\s+"
    r"(?:FUNC|OBJECT|NOTYPE)\s+\S+\s+\S+\s+\S+\s+\S+\s*$",
    re.MULTILINE,
)
MAP_SYMBOL_LINE_PATTERN = re.compile(
    r"^\s*0x[0-9A-Fa-f]{8,16}\s+[A-Za-z_.$][A-Za-z0-9_.$@]*\s*$",
    re.MULTILINE,
)

GIT_IDENTITY_PATTERN = re.compile(
    r"^(author|committer|tagger) (.+) <([^<>]+)> ([0-9]+) ([+-][0-9]{4})$"
)


def run_git(root: Path, arguments: list[str], *, text: bool = True) -> str | bytes:
    completed = subprocess.run(
        ["git", "-C", str(root), *arguments],
        check=True,
        capture_output=True,
        text=text,
    )
    return completed.stdout


def candidate_paths(root: Path) -> list[Path]:
    output = run_git(
        root,
        ["ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        text=False,
    )
    assert isinstance(output, bytes)
    return [
        root / item.decode("utf-8", errors="strict")
        for item in output.split(b"\0")
        if item
    ]


def scan_relative_path(relative: str, *, label: str | None = None) -> list[str]:
    """Apply path-only policy checks to a repository-relative path."""
    normalized = relative.replace("\\", "/")
    lowered = normalized.casefold()
    display = label or normalized
    errors: list[str] = []

    if PurePosixPath(lowered).name in FORBIDDEN_BASENAMES:
        errors.append(f"{display}: submodule configuration may not be tracked")
    if any(lowered.startswith(prefix) for prefix in FORBIDDEN_PREFIXES):
        errors.append(f"{display}: private, generated, or external path may not be tracked")
    if PurePosixPath(normalized).suffix.casefold() in FORBIDDEN_SUFFIXES:
        errors.append(
            f"{display}: forbidden ROM-derived, diagnostic, or archive file extension"
        )
    return errors


def scan_index_entries(root: Path) -> list[str]:
    """Reject gitlinks even when their working-tree path does not exist."""
    output = run_git(root, ["ls-files", "--stage", "-z"], text=False)
    assert isinstance(output, bytes)
    errors: list[str] = []
    for entry in output.split(b"\0"):
        if not entry:
            continue
        metadata, raw_path = entry.split(b"\t", 1)
        mode = metadata.split(b" ", 1)[0]
        if mode == b"160000":
            path = raw_path.decode("utf-8", errors="replace")
            errors.append(f"{path}: gitlinks/submodules may not be tracked")
    return errors


def archive_signature(data: bytes) -> str | None:
    for archive_type, signatures in ARCHIVE_SIGNATURES:
        if any(data.startswith(signature) for signature in signatures):
            return archive_type
    if len(data) >= 262 and data[257:262] == b"ustar":
        return "tar"
    if len(data) >= 0x8006 and data[0x8001:0x8006] == b"CD001":
        return "ISO 9660"
    return None


def _scan_blob(data: bytes, label: str, *, enforce_size: bool = True) -> list[str]:
    errors: list[str] = []
    if enforce_size and len(data) > MAX_TRACKED_BYTES:
        errors.append(
            f"{label}: tracked file is {len(data)} bytes; limit is {MAX_TRACKED_BYTES}"
        )

    if any(magic in data for magic in N64_MAGICS):
        errors.append(f"{label}: contains an N64 ROM byte-order marker")

    if hashlib.sha1(data).hexdigest() in KNOWN_PROTECTED_SHA1:
        errors.append(f"{label}: matches a known protected ROM hash")

    detected_archive = archive_signature(data)
    if detected_archive is not None:
        errors.append(f"{label}: begins with a {detected_archive} archive signature")

    if any(marker in data for marker in PRIVATE_CORPUS_MARKERS):
        errors.append(f"{label}: contains a private-corpus body marker")

    if hashlib.sha256(data).hexdigest() in APPROVED_UI_FONT_SHA256 | APPROVED_UI_IMAGE_SHA256:
        return errors

    if b"\0" in data:
        errors.append(f"{label}: binary content is not allowlisted")
        return errors

    text = data.decode("utf-8", errors="replace")
    for pattern in TEXT_SECRET_PATTERNS:
        if pattern.search(text):
            errors.append(f"{label}: possible credential or private key")
    local_path_text = text
    for marker in SAFE_LOCAL_PATH_MARKERS:
        local_path_text = local_path_text.replace(marker.decode("ascii"), "")
    for pattern in LOCAL_PATH_PATTERNS:
        if pattern.search(local_path_text) and not (
            label.startswith("git-blob:")
            and hashlib.sha256(data).hexdigest() in HISTORICAL_CHECKOUT_PATH_SHA256
        ):
            errors.append(f"{label}: contains a user-specific local path")
    for pattern in GENERATED_TEXT_PATTERNS:
        if pattern.search(text):
            errors.append(f"{label}: contains a generated-recompilation marker")
    if LONG_OPAQUE_BASE64_PATTERN.search(text):
        errors.append(f"{label}: contains a long opaque base64 payload")
    if CHUNKED_OPAQUE_PAYLOAD_PATTERN.search(text):
        errors.append(f"{label}: contains a chunked opaque payload")
    if REPEATED_OPAQUE_TOKEN_PATTERN.search(text):
        errors.append(f"{label}: contains repeated opaque payload chunks")
    if REPEATED_IDENTICAL_OPAQUE_TOKEN_PATTERN.search(text):
        errors.append(f"{label}: contains repeated identical opaque payload chunks")
    if SPACED_HEX_PAYLOAD_PATTERN.search(INVENTORY_IDS_DECLARATION.sub("", text)):
        errors.append(f"{label}: contains a spaced hexadecimal payload")
    try:
        json_value = json.loads(text)
    except json.JSONDecodeError:
        json_value = None
    if json_value is not None:
        forbidden_json_keys = {
            "body",
            "buffer",
            "dmem",
            "imem",
            "raw_bytes",
            "rom_bytes",
            "ucode_body",
        }

        def scan_json(candidate: object, location: str = "$") -> None:
            if isinstance(candidate, dict):
                for key, child in candidate.items():
                    normalized = str(key).casefold().replace("-", "_")
                    if normalized in forbidden_json_keys:
                        errors.append(
                            f"{label}:{location}.{key}: forbidden private-body JSON field"
                        )
                    scan_json(child, f"{location}.{key}")
            elif isinstance(candidate, list):
                def byte_leaf_count(value: object) -> int | None:
                    if type(value) is int:
                        return 1 if 0 <= value <= 255 else None
                    if not isinstance(value, list):
                        return None
                    child_counts = [byte_leaf_count(item) for item in value]
                    if any(count is None for count in child_counts):
                        return None
                    return sum(count for count in child_counts if count is not None)

                byte_count = byte_leaf_count(candidate)
                if byte_count is not None and byte_count >= 64:
                    errors.append(
                        f"{label}:{location}: contains a long byte-range integer array"
                    )
                for index, child in enumerate(candidate):
                    scan_json(child, f"{location}[{index}]")

        scan_json(json_value)
    if len(READELF_SYMBOL_LINE_PATTERN.findall(text)) >= 4:
        errors.append(f"{label}: contains a raw readelf symbol export")
    if len(MAP_SYMBOL_LINE_PATTERN.findall(text)) >= 4:
        errors.append(f"{label}: contains a raw linker-map symbol export")
    return errors


def scan_blob(data: bytes, label: str, *, enforce_size: bool = True) -> list[str]:
    errors = _scan_blob(data, label, enforce_size=enforce_size)
    reviewed_data = data.replace(b"\r\n", b"\n") if b"\0" not in data else data
    reviewed = REVIEWED_BY_SHA256.get(hashlib.sha256(reviewed_data).hexdigest())
    if reviewed is None or len(reviewed_data) != reviewed["bytes"]:
        return errors
    allowed = reviewed["exceptions"]
    return [error for error in errors if not (
        ("size" in allowed and error.startswith(label + ": tracked file is "))
        or ("binary" in allowed and error == label + ": binary content is not allowlisted")
        or any(error == label + ": " + kind for kind in allowed if kind not in ("size", "binary"))
    )]


def scan_path(root: Path, path: Path) -> list[str]:
    relative = path.relative_to(root).as_posix()
    errors = scan_relative_path(relative)
    if not path.is_file():
        return errors

    try:
        data = path.read_bytes()
    except OSError as error:
        errors.append(f"{relative}: could not read file: {error}")
        return errors
    errors.extend(scan_blob(data, relative))
    return errors


def scan_git_metadata(data: bytes, label: str, *, object_type: str) -> list[str]:
    """Preserve contributor identities; enforce maintainer privacy and UTC policy."""
    errors = scan_blob(data, label, enforce_size=False)
    text = data.decode("utf-8", errors="replace")
    expected_roles = {"author", "committer"} if object_type == "commit" else {"tagger"}
    observed_roles: set[str] = set()

    # Merge commits (two or more parents, before the blank line that separates
    # the header from the message) are created by the hosting platform under the
    # merging user's account and cannot carry the generic bot identity. Their
    # content is still scanned; only the identity/UTC rule is relaxed for them.
    header, _, _ = text.partition("\n\n")
    is_merge_commit = (
        object_type == "commit"
        and sum(1 for line in header.splitlines() if line.startswith("parent ")) >= 2
    )

    for line in header.splitlines():
        if not line.startswith(("author ", "committer ", "tagger ")):
            continue
        match = GIT_IDENTITY_PATTERN.fullmatch(line)
        if match is None:
            errors.append(f"{label}: malformed Git identity metadata")
            continue
        role, name, email, _timestamp, offset = match.groups()
        observed_roles.add(role)
        if is_merge_commit:
            continue
        if not approved_git_identity(name, email):
            errors.append(
                f"{label}: {role} must use a valid contributor identity or an approved maintainer no-reply identity"
            )
        if (name, email) in ALLOWED_GIT_IDENTITIES and offset != "+0000":
            errors.append(f"{label}: {role} timestamp must use UTC (+0000)")

    missing_roles = expected_roles - observed_roles
    for role in sorted(missing_roles):
        errors.append(f"{label}: missing {role} identity metadata")
    return errors


def scan_reachable_history(root: Path) -> list[str]:
    errors: list[str] = []
    output = run_git(
        root,
        [
            "cat-file",
            "--batch-all-objects",
            "--batch-check=%(objectname) %(objecttype) %(objectsize)",
        ],
    )
    assert isinstance(output, str)
    for line in output.splitlines():
        object_id, object_type, size_text = line.split()
        size = int(size_text)
        if object_type != "blob":
            continue
        if size > MAX_HISTORY_SCAN_BYTES and object_id not in REVIEWED_LARGE_GIT_BLOBS:
            errors.append(
                f"git-blob:{object_id}: historical blob is {size} bytes; "
                f"scan limit is {MAX_HISTORY_SCAN_BYTES}"
            )
            continue
        data = run_git(root, ["cat-file", "blob", object_id], text=False)
        assert isinstance(data, bytes)
        errors.extend(
            scan_blob(data, f"git-blob:{object_id}", enforce_size=True)
        )

    commits = run_git(root, ["rev-list", "--all"])
    assert isinstance(commits, str)
    historical_paths: set[str] = set()
    for commit in commits.splitlines():
        commit_data = run_git(root, ["cat-file", "commit", commit], text=False)
        assert isinstance(commit_data, bytes)
        errors.extend(
            scan_git_metadata(commit_data, f"git-commit:{commit}", object_type="commit")
        )

        tree = run_git(root, ["ls-tree", "-rz", commit], text=False)
        assert isinstance(tree, bytes)
        for entry in tree.split(b"\0"):
            if not entry:
                continue
            metadata, raw_path = entry.split(b"\t", 1)
            mode = metadata.split(b" ", 1)[0]
            relative = raw_path.decode("utf-8", errors="strict")
            historical_paths.add(relative)
            if mode == b"160000":
                errors.append(
                    f"git-history:{relative}: gitlinks/submodules may not be tracked"
                )

    tag_refs = run_git(
        root,
        ["for-each-ref", "--format=%(objectname)%00%(objecttype)%00%(refname)", "refs/tags"],
        text=False,
    )
    assert isinstance(tag_refs, bytes)
    for record in tag_refs.splitlines():
        if not record:
            continue
        object_id_raw, object_type_raw, ref_name_raw = record.split(b"\0", 2)
        ref_name = ref_name_raw.decode("utf-8", errors="replace")
        errors.extend(scan_blob(ref_name_raw, f"git-ref:{ref_name}", enforce_size=False))
        if object_type_raw != b"tag":
            continue
        object_id = object_id_raw.decode("ascii")
        tag_data = run_git(root, ["cat-file", "tag", object_id], text=False)
        assert isinstance(tag_data, bytes)
        errors.extend(
            scan_git_metadata(tag_data, f"git-tag:{object_id}", object_type="tag")
        )
    for relative in sorted(historical_paths):
        errors.extend(
            scan_relative_path(relative, label=f"git-history:{relative}")
        )
    return errors


def check_repository(root: Path, *, history: bool = False) -> list[str]:
    errors: list[str] = []
    for path in candidate_paths(root):
        errors.extend(scan_path(root, path))
    errors.extend(scan_index_entries(root))
    if history:
        errors.extend(scan_reachable_history(root))
    return sorted(set(errors))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--history", action="store_true")
    arguments = parser.parse_args()

    errors = check_repository(arguments.root.resolve(), history=arguments.history)
    if errors:
        print("Repository hygiene check failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print("Repository hygiene check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
