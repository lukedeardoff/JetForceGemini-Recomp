import unittest
from scripts import native_mmio_access as mmio


def source(duplicate=False):
    result = '#include "funcs.h"\n'
    for pc, primary, base, reg in ((0x80098EC8, 0x23, 10, 10), (0x80098F0C, 0x2b, 1, 10)):
        word = (primary << 26) | (base << 21) | (reg << 16) | 12
        prior = (0x0f << 26) | (base << 16) | 0xa430
        body = (f'ctx->r{reg} = U64(MEM_W(ctx->r{base}, 0XC));' if primary == 0x23 else
                f'MEM_W(0XC, ctx->r{base}) = (int32_t)(ctx->r{reg});')
        if duplicate:
            result += f'// native 0x{pc:08X}: synthetic memory operation\n{body}\n'
        result += f'// 0x{pc-4:08X}: synthetic address setup\n'
        result += f'jfg_phase9_execution_probe_predecoded(0x00000000U, 0x{pc-4:08x}U, 0x{prior:08x}U, ctx);\n'
        result += f'// 0x{pc:08X}: synthetic memory operation\n'
        result += f'jfg_phase9_execution_probe_predecoded(0x00000000U, 0x{pc:08x}U, 0x{word:08x}U, ctx);\n{body}\n'
    return result


class NativeMmioAccessTests(unittest.TestCase):
    def test_supported_pair_preserves_hooks(self):
        original = source()
        changed, count = mmio.rewrite(original)
        self.assertEqual(count, 2)
        self.assertEqual(mmio.HOOK.findall(original), mmio.HOOK.findall(changed))
        self.assertNotIn('MEM_W(', changed)
        self.assertIn('U64(ctx->r10 + 12U)', changed)

    def test_native_and_fallback(self):
        changed, count = mmio.rewrite(source(True))
        self.assertEqual(count, 4)
        self.assertEqual(len(mmio.HOOK.findall(changed)), 4)

    def test_unrelated_source(self):
        self.assertEqual(mmio.rewrite('unrelated'), ('unrelated', 0))

    def test_fail_closed_encodings_and_body(self):
        original = source()
        for old, new in [('3c0aa430', '3c0aa431'), ('8d4a000c', '8d4a000d'),
                         ('ac2a000c', 'ac4a000c'), ('MEM_W(ctx->r10, 0XC)', 'MEM_H(ctx->r10, 0XC)'),
                         ('ctx->r10 = U64(MEM_W', 'ctx->r11 = U64(MEM_W'),
                         ('0x80098f0cU', '0x80098f10U')]:
            with self.subTest(old=old), self.assertRaises(ValueError):
                mmio.rewrite(original.replace(old, new))

    def test_no_double_application(self):
        changed, _ = mmio.rewrite(source())
        with self.assertRaises(ValueError):
            mmio.rewrite(changed)

    def test_reject_extra_memory_access(self):
        with self.assertRaises(ValueError):
            mmio.rewrite(source() + '\nMEM_W(0XC, ctx->r1) = 0;\n')


if __name__ == '__main__':
    unittest.main()
