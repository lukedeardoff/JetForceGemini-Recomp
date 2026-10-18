"""Compile the generated lookup ABI; exercise lifecycle and continuation semantics."""
from pathlib import Path
import os,shutil,subprocess,tempfile,unittest
from scripts.build_private_generated_root import make_lookup,make_lookup_function,make_lookup_entry_header
ROOT=Path(__file__).resolve().parents[1]

class GeneratedEntryMetadataTests(unittest.TestCase):
 def test_entry_identity_survives_rebase_unload_collision_and_reload(self):
  pinned=ROOT/'tools/build/llvm-22.1.8/bin/clang.exe'
  compiler=os.environ.get('JFG_TEST_C_COMPILER') or (str(pinned) if pinned.exists() else shutil.which('clang') or shutil.which('gcc'))
  if not compiler:self.skipTest('C compiler unavailable')
  base=ROOT/'build/test-tmp';base.mkdir(parents=True,exist_ok=True)
  with tempfile.TemporaryDirectory(prefix='entry-metadata-',dir=base) as temp:
   p=Path(temp)
   (p/'funcs.h').write_text('#pragma once\n#include <stdint.h>\n#include <stddef.h>\ntypedef struct { uint32_t entry; } recomp_context;\ntypedef void recomp_func_t(uint8_t*,recomp_context*);\nextern int32_t* section_addresses;\nextern int jfg_generated_lookup_dirty;\nrecomp_func_t* jfg_generated_lookup_function(int32_t);\nuint32_t jfg_generated_lookup_entry_offset(int32_t,recomp_func_t**);\n')
   rows=[dict(section=0,offset=0x100,name='first'),dict(section=1,offset=0x100,name='second')]
   continuations=[dict(section=0,offset=0x120,name='first'),dict(section=0,offset=0x140,name='first'),dict(section=1,offset=0x120,name='second')]
   (p/'lookup.c').write_text(make_lookup(rows,continuations))
   (p/'wrapper.c').write_text(make_lookup_function())
   (p/'generated_lookup_entry.h').write_text(make_lookup_entry_header())
   (p/'driver.c').write_text('#include "funcs.h"\n#include <assert.h>\nstatic int32_t bases[2]; int32_t* section_addresses=bases;\nvoid first(uint8_t* ram,recomp_context* ctx){ram[0]=(uint8_t)(ctx->entry==0x120?2:ctx->entry==0x140?3:1);}\nvoid second(uint8_t* ram,recomp_context* ctx){ram[0]=(uint8_t)(ctx->entry==0x120?5:4);}\nstatic void expect(uint32_t address,recomp_func_t* fn,uint32_t entry,uint8_t result){\n assert(jfg_generated_lookup_function((int32_t)address)==fn);\n assert(jfg_generated_lookup_entry_offset((int32_t)address,NULL)==entry);\n if(fn){uint8_t ram[1]={0};recomp_context ctx={entry};fn(ram,&ctx);assert(ram[0]==result);}\n}\nint main(void){\n bases[0]=(int32_t)0x80000000u;bases[1]=0x100000;jfg_generated_lookup_dirty=1;\n expect(0x80000100u,first,0,1);expect(0x80000120u,first,0x120,2);expect(0x80000140u,first,0x140,3);\n expect(0x100100u,second,0,4);expect(0x100120u,second,0x120,5);expect(0x80000124u,NULL,0,0);\n for(uint32_t i=0;i<1000;++i){\n  uint32_t base=0x200000u+i*0x1000u;bases[1]=(int32_t)base;jfg_generated_lookup_dirty=1;\n  expect(base+0x100u,second,0,4);expect(base+0x120u,second,0x120,5);expect(0x100120u,NULL,0,0);\n  bases[1]=0;jfg_generated_lookup_dirty=1;expect(base+0x100u,NULL,0,0);expect(base+0x120u,NULL,0,0);\n }\n /* Exact address collision follows existing first-published table order. */\n bases[1]=bases[0];jfg_generated_lookup_dirty=1;expect(0x80000100u,first,0,1);expect(0x80000120u,first,0x120,2);\n bases[0]=0;jfg_generated_lookup_dirty=1;expect(0x80000100u,second,0,4);expect(0x80000120u,second,0x120,5);expect(0x80000140u,NULL,0,0);\n bases[0]=(int32_t)0x80000000u;jfg_generated_lookup_dirty=1;expect(0x80000140u,first,0x140,3);\n return 0;\n}\n')
   exe=p/'entries.exe'
   built=subprocess.run([compiler,'-std=c11','-O2','-Wall','-Wextra','-Werror',str(p/'lookup.c'),str(p/'wrapper.c'),str(p/'driver.c'),'-o',str(exe)],capture_output=True,text=True,timeout=90)
   self.assertEqual(built.returncode,0,built.stderr)
   ran=subprocess.run([str(exe)],capture_output=True,text=True,timeout=20)
   self.assertEqual(ran.returncode,0,ran.stderr)


class GeneratedEntryAuditTests(unittest.TestCase):
 def test_lookup_partition_expansion_is_specific_to_entry_metadata(self):
  import json
  from scripts import audit_generated_objects as audit
  document={key:[] for key in (*audit.LIST_KEYS,*audit.ROLE_PARTITION_KEYS)}
  document.update(version=2,section_count=1,coverage_symbol_count=0,coverage_sha256='0'*64)
  document['section_address_support_symbols']=['section_a','section_b','section_c']
  document['lifecycle_support_symbols']=['lifecycle']
  document['relocation_support_symbols']=['reloc_a','reloc_b','reloc_c','reloc_d','reloc_e']
  with tempfile.TemporaryDirectory() as temp:
   path=Path(temp)/'inventory.json'
   for extra,accepted in [(None,True),('jfg_generated_lookup_entry_offset',True),('unexpected_lookup',False)]:
    document['lookup_support_symbols']=sorted(['lookup_a','lookup_b']+([extra] if extra else []))
    all_roles=['section_address_support_symbols','lookup_support_symbols','lifecycle_support_symbols','relocation_support_symbols']
    combined=sorted(symbol for role in all_roles for symbol in document[role])
    document['table_support_symbols']=combined;document['support_lifecycle_symbols']=combined
    path.write_text(json.dumps(document))
    if accepted:self.assertEqual(audit._load_inventory(path),document)
    else:
     with self.assertRaisesRegex(audit.AuditFailure,'support-role-partition-mismatch'):audit._load_inventory(path)
