"""Consolidate native timing bookkeeping, retaining every guest operation.

Apply only after normalization has placed every alternate-entry label. Spans
contain no branches, CPU-control operations, callbacks, or C control flow. RAM
operations receive a dynamic cached-RDRAM/alignment guard before execution;
other addresses commit the preceding prefix and resume individual timing hooks.
The original timing path retains its exact hook/instruction sequence.
"""
from pathlib import Path
import re

HOOK = re.compile(r'jfg_phase9_execution_probe_predecoded\(0x([0-9a-f]+)U, 0x([0-9a-f]+)U, 0x([0-9a-f]+)U, ctx\);', re.I)
COMMENT = re.compile(r'(?m)^([ \t]*)// 0x([0-9a-f]{8}):[^\n]*\n', re.I)
MEMORY_WIDTH = {0x20:1,0x21:2,0x23:4,0x24:1,0x25:2,0x27:4,0x28:1,0x29:2,0x2b:4,0x31:4,0x35:8,0x37:8,0x39:4,0x3d:8,0x3f:8}
INTEGER_SPECIAL = {0,2,3,4,6,7,16,17,18,19,20,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,42,43,44,45,46,47,56,58,59,60,62,63}
MARKER = 'JFG_NATIVE_TIMING_SPAN'


def instruction_kind(metadata: int, word: int) -> tuple[bool, int]:
    if metadata & 0xe00 or (metadata >> 12) & 3 == 3:
        return False, 0
    primary, rs, special = word >> 26, (word >> 21) & 31, word & 63
    width = MEMORY_WIDTH.get(primary, 0)
    integer = primary in (8,9,10,11,12,13,14,15,24,25) or primary == 0 and special in INTEGER_SPECIAL
    floating = primary == 17 and rs in (0,1,2,4,5,16,17,20,21)
    return bool(integer or floating or width), width



def memory_guard_address(body: str, word: int, width: int) -> str | None:
    """Prove and return the actual generated address, including relocation.

    MEM/LD add both operands; SD is (value, offset, base). Require the decoded
    base register and either its exact literal offset or a signed, pure
    RELOC_LO16 with two literal arguments. Never guard the ROM offset while
    executing a different relocated address. Unknown forms stay unbatched.
    """
    matches = list(re.finditer(r'\b(MEM_W|MEM_H|MEM_HU|MEM_B|MEM_BU|LD|SD)\s*\(', body))
    if len(matches) != 1:
        return None
    match = matches[0]
    if {'MEM_W':4, 'MEM_H':2, 'MEM_HU':2, 'MEM_B':1, 'MEM_BU':1, 'LD':8, 'SD':8}[match[1]] != width:
        return None
    depth = 0
    start = match.end()
    args = []
    for index in range(start, len(body)):
        char = body[index]
        if char == '(':
            depth += 1
        elif char == ')':
            if depth == 0:
                args.append(body[start:index].strip())
                break
            depth -= 1
        elif char == ',' and depth == 0:
            args.append(body[start:index].strip())
            start = index + 1
    else:
        return None
    if match[1] == 'SD':
        if len(args) != 3:
            return None
        args = args[1:]
    elif len(args) != 2:
        return None
    register = (word >> 21) & 31
    offset = word & 0xffff
    if offset & 0x8000:
        offset -= 0x10000

    def literal(value):
        value = re.sub(r'^U64\(S64\((.*)\)\)$', r'\1', value)
        if not re.fullmatch(r'-?(?:0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*', value):
            return None
        try:
            return int(re.sub(r'[uUlL]+$', '', value), 0)
        except ValueError:
            return None

    for base, immediate in (args, args[::-1]):
        base_matches = base == f'ctx->r{register}' if register else literal(base) == 0
        if not base_matches:
            continue
        if literal(immediate) == offset:
            return f'(uint64_t)({base}) + (uint64_t)(int64_t)(int16_t)0x{word & 0xffff:04x}U'
        number=r'(?:0[xX][0-9a-fA-F]+|[0-9]+)'
        if re.fullmatch(r'\(int16_t\)\s*RELOC_LO16\(\s*'+number+r'\s*,\s*'+number+r'\s*\)',immediate):
            return f'(uint64_t)({base}) + (uint64_t)(int64_t)({immediate})'
    return None


def memory_expression_matches(body: str, word: int, width: int) -> bool:
    return memory_guard_address(body, word, width) is not None


def rewrite_source(text: str) -> tuple[str, dict[str, int]]:
    if MARKER in text:
        raise ValueError('Native spans require fresh normalized source')
    starts = list(COMMENT.finditer(text))
    records = []
    for i, start in enumerate(starts):
        end = starts[i+1].start() if i+1 < len(starts) else len(text)
        part = text[start.end():end]
        hook = HOOK.match(part.lstrip())
        if not hook:
            records.append(None)
            continue
        leading = len(part)-len(part.lstrip())
        metadata, pc, word = (int(v,16) for v in hook.groups())
        if pc != int(start[2],16):
            raise ValueError('Disassembly and timing hook PC disagree')
        body = part[leading+hook.end():]
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', body, flags=re.S)
        allowed, width = instruction_kind(metadata, word)
        # No labels or scope changes; no declarations whose visibility could
        # escape a span. MUL_S/MUL_D are plain floating-point multiplication;
        # preserve their expressions and existing FPU eligibility checks.
        # Existing helpers are arithmetic only. RELOC_HI16 and
        # RELOC_LO16 read the host section table; only excluded calls can update
        # that table. Memory guards below prove the actual generated address,
        # including signed relocation operands; unknown forms stay unbatched.
        forbidden = re.search(r'[{}#]|\b(goto|return|switch|if|while|for|jfg_\w*)\b|^\s*[A-Za-z_]\w*:', clean, re.M)
        calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', clean))
        known = {'MUL_S','MUL_D','RELOC_HI16','RELOC_LO16','SIGNED','U64','U32','S64','S32','ADD32','SUB32','MEM_W','MEM_H','MEM_HU','MEM_B','MEM_BU','LD','SD','CHECK_FR','CHECK_NAN','NAN_CHECK','DIV_S','DIV_U','DDIV','DDIVU','DMULT','DMULTU','MUL64','sqrtf','sqrt','fabsf','fabs','roundf','round','truncf','trunc','floorf','floor','ceilf','ceil','cvt_w_s','cvt_w_d','cvt_l_s','cvt_l_d','CVT_W_S','CVT_W_D','CVT_L_S','CVT_L_D','isnan'}
        declares = re.search(r'^\s*(?:u?int(?:8|16|32|64)_t|gpr|bool|const|float|double|int|unsigned|signed|long|short|char|auto)\s+[A-Za-z_]\w*', clean, re.M)
        if forbidden or declares or calls-known or (width and not memory_expression_matches(clean, word, width)):
            allowed = False
        records.append({'start':start.start(),'end':end,'metadata':metadata,'pc':pc,'word':word,'width':width,'allowed':allowed,'comment':text[start.start():start.end()],'hook':hook[0],'body':body})
    groups = []; current = []
    for record in records:
        if record is None or not record['allowed'] or current and (record['pc'] != current[-1]['pc']+4 or len(current)==16):
            if len(current)>=2: groups.append(current)
            current=[]
        if record is not None and record['allowed']: current.append(record)
    if len(current)>=2: groups.append(current)
    for group_index, group in reversed(list(enumerate(groups))):
        flags = 0x100 if any(r['metadata']&0x100 for r in group) else 0
        first = group[0]['pc']
        label = f'jfg_native_span_{group_index}'
        lines = ['    { /* '+MARKER+' */\n', f'    if (jfg_native_timing_span_begin(0x{flags:x}U, {len(group)}U, ctx)) {{\n']
        # Keep the admitted native block contiguous. Intermediate register writes
        # need not remain observable to an impossible instruction callback.
        # A failed RAM guard commits its executed prefix before entering the
        # exact original hook at that instruction, then falls through the tail.
        for i, record in enumerate(group):
            lines.append(re.sub(r'// 0x', '// native 0x', record['comment'], flags=re.I))
            if record['width']:
                address=memory_guard_address(record['body'],record['word'],record['width'])
                assert address is not None
                lines.append(f'    if (!jfg_native_timing_span_ram({address}, {record["width"]}U)) {{\n')
                if i:
                    previous=group[i-1]
                    lines.append(f'        jfg_native_timing_span_commit(0x{first:08x}U, 0x{previous["pc"]:08x}U, 0x{previous["word"]:08x}U, {i}U);\n')
                lines.append(f'        goto {label}_fallback_{i};\n    }}\n')
            lines.append(record['body'])
        last=group[-1]
        lines.append(f'    jfg_native_timing_span_commit(0x{first:08x}U, 0x{last["pc"]:08x}U, 0x{last["word"]:08x}U, {len(group)}U);\n    goto {label}_done;\n    }}\n')
        for i, record in enumerate(group):
            if record['width']:
                lines.append(f'{label}_fallback_{i}:\n')
            lines.append(record['comment']+'    '+record['hook']+record['body'])
        lines.append(f'{label}_done:;\n    }}\n')
        text=text[:group[0]['start']]+''.join(lines)+text[group[-1]['end']:]
    return text, {'spans':len(groups),'instructions':sum(map(len,groups)),'memory_guards':sum(bool(r['width']) for g in groups for r in g)}


def rewrite_tree(root: Path) -> dict[str,int]:
    totals={'files':0,'spans':0,'instructions':0,'memory_guards':0}
    for path in sorted((root/'baseline').glob('*_recomp.c')):
        before=path.read_text();after,counts=rewrite_source(before)
        if after!=before:
            path.write_text(after);totals['files']+=1
        for key,value in counts.items():totals[key]+=value
    return totals
