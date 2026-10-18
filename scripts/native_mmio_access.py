"""Qualify two SDK interrupt-mask accesses without changing guest timing.

Only supported instruction sites, adjacent LUI and exact generated memory
expressions are admitted. Hooks, span guards and instruction costs remain.
The runtime bridge independently checks the full address and device state.
"""
from pathlib import Path
import hashlib
import re

SITES = {0x80098EC8: 0x23, 0x80098F0C: 0x2B}
MARKER = '/* JFG qualified MI-mask accesses, schema 1. */'
PROTOTYPES = """\n#ifdef __cplusplus
extern "C" {
#endif
int32_t jfg_mmio_mask_read32(uint8_t*, void*, uint64_t);
void jfg_mmio_mask_write32(uint8_t*, void*, uint64_t, uint32_t);
#ifdef __cplusplus
}
#endif
"""
HOOK = re.compile(r'jfg_phase9_execution_probe_predecoded\(0x[0-9a-f]+U, 0x([0-9a-f]+)U, 0x([0-9a-f]+)U, ctx\);', re.I)
COMMENT = re.compile(r'^\s*// (?:native )?0x([0-9a-f]{8}):[^\n]*', re.I | re.M)


def rewrite(text: str) -> tuple[str, int]:
    if MARKER in text:
        raise ValueError('MI-mask generation already applied')
    hooks = {}
    for match in HOOK.finditer(text):
        hooks.setdefault(int(match[1], 16), []).append(int(match[2], 16))
    present = set(SITES) & hooks.keys()
    if not present:
        return text, 0
    if present != set(SITES):
        raise ValueError('Incomplete supported MI-mask instruction pair')
    edits = []
    comments = list(COMMENT.finditer(text))
    for pc, primary in SITES.items():
        if len(hooks[pc]) != 1 or len(hooks.get(pc - 4, [])) != 1:
            raise ValueError('Missing or duplicated MI-mask timing hook')
        word, prior = hooks[pc][0], hooks[pc - 4][0]
        base, reg = (word >> 21) & 31, (word >> 16) & 31
        if (word >> 26 != primary or word & 0xffff != 12 or not base or not reg or
                prior >> 26 != 0x0f or (prior >> 16) & 31 != base or
                (prior >> 21) & 31 or prior & 0xffff != 0xa430):
            raise ValueError('Unsupported MI-mask instruction encoding')
        instances = [i for i, comment in enumerate(comments) if int(comment[1], 16) == pc]
        if len(instances) not in (1, 2):
            raise ValueError('Unexpected MI-mask generated instruction count')
        for i in instances:
            start = comments[i].end()
            end = comments[i + 1].start() if i + 1 < len(comments) else len(text)
            body = text[start:end]
            if primary == 0x23:
                pattern = rf'ctx->r{reg} = U64\(MEM_W\(ctx->r{base}, 0XC\)\);'
                replacement = f'ctx->r{reg} = U64(jfg_mmio_mask_read32(rdram, ctx, U64(ctx->r{base} + 12U)));'
            else:
                pattern = rf'MEM_W\(0XC, ctx->r{base}\) = \(int32_t\)\(ctx->r{reg}\);'
                replacement = f'jfg_mmio_mask_write32(rdram, ctx, U64(ctx->r{base} + 12U), (uint32_t)(ctx->r{reg}));'
            body, count = re.subn(pattern, lambda _: replacement, body)
            if count != 1 or 'MEM_W(' in body:
                raise ValueError('Unsupported MI-mask generated memory expression')
            edits.append((start, end, body))
    for start, end, body in sorted(edits, reverse=True):
        text = text[:start] + body + text[end:]
    include = '#include "funcs.h"'
    if text.count(include) != 1:
        raise ValueError('Expected one generated function header')
    return text.replace(include, include + '\n' + MARKER + PROTOTYPES, 1), len(edits)


def rewrite_tree(root: Path) -> dict:
    pending = []
    for path in sorted((root / 'baseline').glob('*.c')):
        changed, count = rewrite(path.read_text())
        if count:
            pending.append((path, changed, count))
    if len(pending) != 1:
        raise ValueError('Expected exactly one qualified interrupt-mask function')
    for path, changed, _ in pending:
        path.write_text(changed)
    return {'schema': 1, 'qualified_sites': 2,
            'generated_accesses': sum(count for _, _, count in pending),
            'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'timing_hooks_preserved': True}
