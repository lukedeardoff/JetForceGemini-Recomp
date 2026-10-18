import hashlib
import unittest
from scripts.native_timing_spans import rewrite_source
from scripts.prepare_native_collision import Source, extract

def fixture(extra=""):
    lines=[]
    for index in range(4):
        pc=0x80010000+index*4
        lines.append(
            f"    // 0x{pc:08X}: synthetic\n"
            f"    jfg_phase9_execution_probe_predecoded(0x00000000U, 0x{pc:08x}U, 0x24210001U, ctx);\n"
            "    ctx->r1 = U64(ADD32(ctx->r1, 1));\n")
    text=('#include "recomp.h"\nRECOMP_FUNC void synthetic(uint8_t* rdram, recomp_context* ctx) {\n'
          + "".join(lines)+extra+"\n}\n")
    rewritten, counts=rewrite_source(text)
    spec=Source("bsp","synthetic.c","synthetic",
                hashlib.sha256(rewritten.encode()).hexdigest(),counts["spans"])
    return rewritten,spec

class NativeCollisionSourceTests(unittest.TestCase):
    def test_fallback_operations_are_retained_exactly_once(self):
        source,spec=fixture()
        result=extract(source,spec)
        self.assertEqual(result.count("ctx->r1 = U64(ADD32(ctx->r1, 1));"),4)
        self.assertIn("RECOMP_FUNC void bsp_core(",result)
        self.assertNotIn("jfg_native_",result)
        self.assertNotIn("jfg_phase9_",result)
        self.assertNotIn("#include",result)

    def test_changed_source_is_not_admitted(self):
        source,spec=fixture()
        with self.assertRaisesRegex(ValueError,"Unqualified"):
            extract(source.replace("ADD32(ctx->r1, 1)","ADD32(ctx->r1, 2)",1),spec)

    def test_unknown_callee_is_rejected_even_with_matching_digest(self):
        source,spec=fixture("    LOOKUP_FUNC(0x00312345)(rdram, ctx);\n")
        with self.assertRaisesRegex(ValueError,"unqualified call"):
            extract(source,spec)

    def test_only_closed_calls_are_rewritten(self):
        source,spec=fixture("    LOOKUP_FUNC(0x00317174)(rdram, ctx);\n")
        result=extract(source,spec)
        self.assertIn("bounded(rdram, ctx);",result)
        self.assertNotIn("LOOKUP_FUNC",result)

    def test_incomplete_span_inventory_is_rejected(self):
        source,spec=fixture()
        wrong=Source(spec.name,spec.filename,spec.symbol,spec.sha256,spec.spans+1)
        with self.assertRaisesRegex(ValueError,"Incomplete"):
            extract(source,wrong)

if __name__=="__main__":
    unittest.main()
