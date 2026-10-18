"""Compare native resident math against locally generated C; keep outputs private.
Run from an x64 MSVC developer environment with the retained generated root.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,subprocess

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--generated-root",type=Path,required=True)
 parser.add_argument("--output",type=Path,required=True)
 args=parser.parse_args()
 root=Path(__file__).resolve().parents[1];generated=args.generated_root.resolve()
 out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
 cl=shutil.which("cl.exe")
 if not cl:parser.error("Run from an x64 MSVC developer environment.")
 shutil.copy2(root/"include/jfg/boot/original_timing_fast.h",out/"timing.h")
 common=["/nologo","/O2","/Ob2","/fp:precise","/DNDEBUG","/MD",
  "/I"+str(out),"/I"+str(generated/"include"),"/I"+str(root/"include")]
 definitions=[
  ("angle","baseline/fn_000_0723_recomp.c","fn_000_0723_recomp","jfg_native_angle_try(ram,ctx)",0),
  ("diff","baseline/fn_000_0724_recomp.c","fn_000_0724_recomp","jfg_native_angle_diff_try(ram,ctx)",1),
  ("sin","support/fn_000_0722.c","fn_000_0722","jfg_native_trig_try(ram,ctx,false)",2),
  ("cos","baseline/fn_000_0721_recomp.c","fn_000_0721_recomp","jfg_native_trig_try(ram,ctx,true)",3)]
 results=[]
 for name,relative,symbol,call,kind in definitions:
  case=out/name;case.mkdir();reference=generated/relative
  source=reference.read_text().replace('#include "funcs.h"','#include "timing.h"')
  if source.count("void "+symbol+"(")!=1:raise ValueError("Unexpected generated reference: "+relative)
  for role in ("reference","original"):(case/(role+".c")).write_text(source.replace(symbol,"math_"+role))
  (case/"candidate.cpp").write_text('#include "timing.h"\n#include "jfg/boot/native_resident_math.h"\nextern "C" void math_original(uint8_t*,recomp_context*);\nextern "C" void math_candidate(uint8_t* ram,recomp_context* ctx) {if(!'+call+')math_original(ram,ctx);}\n')
  with (case/"build.log").open("w") as log:
   for unit in ("reference","original"):
    subprocess.run([cl,*common,"/std:c11","/c",str(case/(unit+".c")),"/Fo"+str(case/(unit+".obj"))],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
   subprocess.run([cl,*common,"/std:c++20","/EHsc","/c",str(case/"candidate.cpp"),"/Fo"+str(case/"candidate.obj")],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
   subprocess.run([cl,*common,"/std:c++20","/EHsc","/DJFG_MATH_FIXTURE_KIND="+str(kind),
     str(root/"tests/native_resident_math_reference.cpp"),
     *[str(case/(x+".obj"))for x in ("reference","original","candidate")],
     "/Fe"+str(case/"fixture.exe"),"/Fo"+str(case/"main.obj")],
     stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
  result=subprocess.run([str(case/"fixture.exe")],capture_output=True,text=True,timeout=120)
  item={"helper":name,"returncode":result.returncode,"output":result.stdout+result.stderr,
        "reference_sha256":hashlib.sha256(reference.read_bytes()).hexdigest()}
  results.append(item);(out/"results.json").write_text(json.dumps(results,indent=2))
  print(json.dumps(item),flush=True);result.check_returncode()
 (out/"result.json").write_text(json.dumps({"passed":True,"cases":400000,"references":"Private locally generated C","cpu_timing_fenv_ram_exact":True},indent=2))
 return 0
if __name__=="__main__":raise SystemExit(main())
