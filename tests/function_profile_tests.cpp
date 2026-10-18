#include "jfg/boot/function_profile.hpp"
#include <cstdint>
using P=jfg::boot::FunctionProfile;
P::Sample at(std::uint64_t value,bool valid=true,std::uint32_t thread=7) {
 return {value*10,value,value/10,value*2,thread,valid};
}
int main() {
 P p;p.reset();p.enter(1,at(0));p.enter(2,at(10));p.enter(3,at(20));
 if(p.leave(at(30)) || p.leave(at(50)) || !p.leave(at(100)) || !p.valid())return 1;
 std::uint64_t exclusive=0;
 for(const auto& x:p.rows()) {
   exclusive+=x.self_cycles;
   if(x.target==2 && (x.cycles!=40 || x.self_cycles!=30 || x.self_wall!=300 || x.calls!=1))return 2;
 }
 if(exclusive!=100 || p.calls()!=3)return 3;
 p.reset();p.enter(1,at(0));p.enter(2,at(10));p.enter(2,at(20));
 p.leave(at(30));p.leave(at(50));p.enter(2,at(60));p.leave(at(70));p.leave(at(100));
 exclusive=0;for(const auto& x:p.rows())exclusive+=x.self_cycles;
 if(exclusive!=100 || p.calls()!=4 || !p.valid())return 4;
 p.reset();p.enter(1,at(0));p.enter(2,at(10));p.leave(at(30,false));p.leave(at(100));
 if(p.valid())return 5;
 p.reset();p.enter(1,at(10));p.leave(at(5));if(p.valid())return 6;
 p.reset();p.enter(1,at(0));p.leave(at(10,true,9));if(p.valid())return 7;
 p.reset();for(unsigned i=0;i<P::max_depth+5;++i)p.enter(i+1,at(i));
 for(unsigned i=0;i<P::max_depth+5;++i)p.leave(at(2000+i));
 if(p.valid() || !p.overflow() || p.active())return 8;
 p.reset();p.enter(1,at(0));p.leave(at(10));
 if(!p.valid() || p.overflow() || p.calls()!=1)return 9;
 p.reset();p.enter(1,at(0));p.enter(2,at(1));p.leave(at(20));p.leave(at(10));
 if(p.valid())return 10;
 return 0;
}
