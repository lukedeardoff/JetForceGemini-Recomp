"""Derive the qualified collision closure from owner-generated private code.

No game instructions are stored in this script. Only the exact qualified
source digests are admitted; changes require fresh differential qualification.
Generated output belongs in the private build tree.
"""
from dataclasses import dataclass
import argparse
import hashlib
import json
from pathlib import Path
import re

@dataclass(frozen=True)
class Source:
    name: str
    filename: str
    symbol: str
    sha256: str
    spans: int

SOURCES = (
    Source("bsp", "fn_003_0168_recomp.c", "fn_003_0168_recomp",
           "01d479153b3c90a5ce32a0eb704e60ad114755b14873df9717871d14685f7bc6", 36),
    Source("split", "fn_003_0167_recomp.c", "fn_003_0167_recomp",
           "fb35a0916260c4cd1780100369133c65e8504cf40b26382a8244e5b18f60199b", 4),
    Source("line", "fn_003_0054_recomp.c", "fn_003_0054_recomp",
           "8a6e73be064d5108544190f3f1a7ec94d053c2e0c61fa3c513b5231f9f607300", 10),
)

SPAN = re.compile(
    r"    \{ /\* JFG_NATIVE_TIMING_SPAN \*/\n.*?"
    r"    goto jfg_native_span_(\d+)_done;\n    \}\n"
    r"(.*?)jfg_native_span_\1_done:;\n    \}", re.S)

def extract(source: str, spec: Source) -> str:
    if hashlib.sha256(source.encode()).hexdigest() != spec.sha256:
        raise ValueError(f"Unqualified collision source: {spec.filename}")
    if source.count("RECOMP_FUNC void " + spec.symbol + "(") != 1:
        raise ValueError("Unexpected collision entry definition")
    body, count = SPAN.subn(lambda match: match[2], source)
    if count != spec.spans or count != source.count("JFG_NATIVE_TIMING_SPAN"):
        raise ValueError("Incomplete collision span extraction")
    body = re.sub(r"^jfg_native_span_\d+_fallback_\d+:\s*\n", "", body, flags=re.M)
    body = re.sub(r"^\s*jfg_phase9_execution_probe_predecoded\([^\n]*\);\n",
                  "\n", body, flags=re.M)
    if "jfg_native_" in body or "jfg_phase9_" in body:
        raise ValueError("Unexpected remaining timing operation")
    body = re.sub(r"^#include.*\n", "", body, flags=re.M)
    body = body.replace(spec.symbol, spec.name + "_core")
    for address, target in (("0x00317174", "bounded"),
                            ("0x003170D0", "split_core"),
                            ("0x00303E90", "line_core")):
        body = body.replace("LOOKUP_FUNC(" + address + ")", target)
    if "LOOKUP_FUNC" in body or re.search(r"\bfn_\w+\s*\(", body):
        raise ValueError("Collision closure contains an unqualified call")
    return body

def generate(root: Path, output: Path) -> dict:
    parts, receipts = [], []
    for spec in SOURCES:
        source = (root / "baseline" / spec.filename).read_text(encoding="utf-8")
        body = extract(source, spec)
        parts.append(body)
        receipts.append({"file": spec.filename, "source_sha256": spec.sha256,
                         "body_sha256": hashlib.sha256(body.encode()).hexdigest()})
    text = "// Private, locally generated collision bodies. Do not publish.\n" + "\n".join(parts)
    receipt = {"schema": 1, "sources": receipts,
               "output_sha256": hashlib.sha256(text.encode()).hexdigest()}
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    temporary.write_text(text, encoding="utf-8", newline="\n")
    temporary.replace(output)
    output.with_suffix(output.suffix + ".json").write_text(
        json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt

def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generated-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        receipt = generate(args.generated_root, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    print(json.dumps(receipt))

if __name__ == "__main__":
    main()
