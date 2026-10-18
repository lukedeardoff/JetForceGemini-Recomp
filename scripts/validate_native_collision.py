"""Qualify native collision against locally generated original C.
Requires an x64 MSVC developer shell. Generated game code stays in --output.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess,sys

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument("--generated-root",type=Path,required=True)
 p.add_argument("--output",type=Path,required=True)
 a=p.parse_args();root=Path(__file__).resolve().parents[1];g=a.generated_root.resolve()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
 cl=shutil.which("cl.exe")
 if not cl:p.error("Run from an x64 MSVC developer shell.")
 subprocess.run([sys.executable,str(root/"scripts/prepare_native_collision.py"),
   "--generated-root",str(g),"--output",str(out/"collision.inc")],check=True,timeout=30)
 shutil.copy2(root/"include/jfg/boot/original_timing_fast.h",out/"timing.h")
 common=["/nologo","/O2","/Ob2","/fp:precise","/DNDEBUG","/MD","/wd4702",
   "/I"+str(out),"/I"+str(g/"include"),"/I"+str(root/"include")]
 references=[];objects=[]
 definitions=[("bsp","fn_003_0168_recomp"),("split","fn_003_0167_recomp"),("line","fn_003_0054_recomp")]
 with(out/"build.log").open("w")as log:
  for name,symbol in definitions:
   raw=(g/"baseline"/(symbol+".c")).read_text()
   source=raw.replace('#include "funcs.h"','#include "timing.h"').replace(symbol,name+"_reference")
   unit=out/(name+"_reference.c");unit.write_text(source)
   obj=out/(name+"_reference.obj");objects.append(str(obj))
   references.append({"helper":name,"normalized_source_sha256":hashlib.sha256(raw.encode()).hexdigest()})
   subprocess.run([cl,*common,"/std:c11","/c",str(unit),"/Fo"+str(obj)],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
  obj=out/"native_collision.obj";objects.append(str(obj))
  subprocess.run([cl,*common,"/std:c++20","/EHs","/DJFG_NATIVE_COLLISION_ENABLED=1",
   '/DJFG_NATIVE_COLLISION_BODY="'+str(out/"collision.inc").replace("\\","/")+'"',
   "/c",str(root/"src/boot/native_collision.cpp"),"/Fo"+str(obj)],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
  subprocess.run([cl,*common,"/std:c++20","/EHs","/DJFG_NATIVE_COLLISION_ENABLED=1",
   str(root/"tests/native_collision_reference.cpp"),*objects,"/Fe"+str(out/"fixture.exe"),
   "/Fo"+str(out/"fixture.obj")],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
 run=subprocess.run([str(out/"fixture.exe")],capture_output=True,text=True,timeout=180)
 result={"passed":run.returncode==0,"returncode":run.returncode,"stdout":run.stdout,"stderr":run.stderr,
   "random_cases":50000,"references":references,"cpu_stack_globals_fenv_exact":run.returncode==0,
   "full_ram_periodically_exact":run.returncode==0,"production_module":True}
 (out/"result.json").write_text(json.dumps(result,indent=2));print(json.dumps(result),flush=True)
 run.check_returncode()
 return 0
if __name__=="__main__":raise SystemExit(main())
