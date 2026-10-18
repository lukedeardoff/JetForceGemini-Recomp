#include "jfg/boot/count_sample_tape.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
using jfg::boot::CountSampleTape;
unsigned checks=0;
void check(bool value,const char* text){++checks;if(!value){std::cerr<<text<<'\n';std::exit(1);}}
int main(){
 CountSampleTape off;check(off.sample(1,2,3,0,99)==99,"off changed Count");check(off.finish(),"off finish");
 std::stringstream stream;CountSampleTape record;check(record.record(stream),"record header");
 for(unsigned i=0;i<1000;++i)check(record.sample(1000+i,0x80010000U+4*i,0x24210001U,i%2?5000+i:0,5000+i)==5000+i,"record sample");
 check(record.samples()==1000&&record.finish(),"record complete");
 const auto bytes=stream.str();CountSampleTape replay;check(replay.replay(stream),"replay header");
 for(unsigned i=0;i<1000;++i)check(replay.sample(1000+i,0x80010000U+4*i,0x24210001U,i%2?5000+i:0,std::nullopt)==5000+i,"replay Count");
 check(replay.samples()==1000&&replay.finish(),"replay complete");check(!replay.sample(1,2,3,0,99),"use after finish");
 for(unsigned field=0;field<4;++field){
  std::stringstream input(bytes);CountSampleTape t;check(t.replay(input),"mismatch header");
  check(!t.sample(field==0?1001:1000,field==1?0x80010004U:0x80010000U,field==2?0U:0x24210001U,field==3?1:0,std::nullopt),"boundary mismatch accepted");check(!t.finish(),"failed replay passed");
 }
 {std::stringstream input(bytes);CountSampleTape t;check(t.replay(input)&&!t.finish(),"unconsumed tail accepted");}
 for(unsigned size: {0U,1U,15U,16U,17U,47U}){
  std::stringstream input(bytes.substr(0,size));CountSampleTape t;const auto ok=t.replay(input);
  check(size<16?!ok:ok,"truncated header status");
  if(ok)check(!t.sample(1000,0x80010000,0x24210001,0,std::nullopt),"truncated row accepted");
 }
 {auto bad=bytes;bad[0]^=1;std::stringstream input(bad);CountSampleTape t;check(!t.replay(input),"wrong magic");}
 {std::stringstream output;CountSampleTape t;check(t.record(output),"nonmonotonic header");check(t.sample(2,4,0,0,100)==100,"first monotonic sample");check(!t.sample(3,4,0,0,99),"backward Count");}
 {std::stringstream output;CountSampleTape t;check(t.record(output),"instruction header");check(t.sample(2,4,0,0,100)==100,"instruction sample");check(!t.sample(1,4,0,0,101),"backward instruction ordinal");}
 {std::stringstream output;CountSampleTape t;check(t.record(output),"idle header");check(!t.sample(1,4,0,101,100),"early idle return");}
 {std::stringstream output;CountSampleTape t;check(t.record(output),"output header");output.setstate(std::ios::badbit);check(!t.sample(1,4,0,0,100)&&!t.finish(),"output failure ignored");}
 {
  std::stringstream input(bytes);CountSampleTape t;
  check(!t.release_prefix(5000),"inactive prefix released");
  check(t.replay(input)&&!t.release_prefix(5000),"empty prefix released");
  check(t.sample(1000,0x80010000,0x24210001,0,std::nullopt)==5000,"prefix first sample");
  check(!t.release_prefix(4999)&&!t.release_prefix(5001),"mismatched Count released");
  check(t.replaying()&&t.release_prefix(5000),"valid prefix refused");
  check(!t.active()&&!t.replaying()&&t.samples()==1&&t.finish(),"released state invalid");
  check(t.sample(1001,0x80010004,0x24210001,0,5003)==5003,"native Count not passed through");
  check(!t.release_prefix(5000)&&!t.replay(input),"prefix restarted");
 }
 {
  std::stringstream input(bytes);CountSampleTape t;check(t.replay(input),"failed prefix header");
  check(!t.sample(1001,0x80010000,0x24210001,0,std::nullopt),"prefix mismatch absent");
  check(!t.release_prefix(5000)&&!t.finish(),"failed prefix released");
 }
 {std::stringstream output;CountSampleTape t;check(t.record(output),"prefix recording header");
  check(t.sample(1,4,0,0,100)==100&&!t.release_prefix(100)&&t.finish(),"recording prefix released");}

 {
  CountSampleTape bounded;std::stringstream output;
  check(!bounded.set_sample_limit(0)&&!bounded.set_sample_limit(64000001U),"invalid sample limits accepted");
  check(bounded.set_sample_limit(64000000U)&&bounded.set_sample_limit(2),"bounded configuration refused");
  check(bounded.record(output)&&!bounded.set_sample_limit(3),"active record budget changed");
  check(bounded.sample(1,4,0,0,100)==100&&bounded.sample(2,8,0,0,101)==101,"bounded record lost samples");
  check(bounded.samples()==2&&bounded.limit_reached(),"limit accounting mismatch");
  check(!bounded.sample(3,12,0,0,102)&&bounded.samples()==2&&!bounded.finish(),"exhausted recording passed");
  check(!bounded.set_sample_limit(4),"exhausted tape reset");
 }
 {
  CountSampleTape bounded;std::stringstream input(bytes);
  check(bounded.set_sample_limit(2)&&bounded.replay(input),"bounded replay setup");
  check(!bounded.set_sample_limit(3),"active replay budget changed");
  for(unsigned i=0;i<2;++i)check(bounded.sample(1000+i,0x80010000U+4*i,0x24210001U,i%2?5000+i:0,std::nullopt)==5000+i,"bounded replay mismatch");
  check(!bounded.sample(1002,0x80010008U,0x24210001U,0,std::nullopt)&&bounded.samples()==2&&bounded.limit_reached()&&!bounded.finish(),"exhausted replay passed");
 }
 {
  CountSampleTape exact;std::stringstream output;
  check(exact.set_sample_limit(1)&&exact.record(output),"exact limit setup");
  check(exact.sample(1,4,0,0,100)==100&&exact.finish(),"exactly sized recording rejected");
  check(!exact.set_sample_limit(2),"finished tape budget changed");
 }
 std::cout<<checks<<" Count sample transcript contracts passed\n";
}
