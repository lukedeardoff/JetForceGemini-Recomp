#include "preset/rt64_preset_draw_call.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <type_traits>
int main() {
 using RT64::DrawCallKey;
 static_assert(std::is_standard_layout_v<DrawCallKey>);
 constexpr auto fields=offsetof(DrawCallKey,geometryMode)+sizeof(DrawCallKey::geometryMode);
 static_assert(fields==84 && sizeof(DrawCallKey)==88);
 unsigned failed=0;
 auto check=[&](bool value,const char* label){if(!value){++failed;std::printf("FAIL %s\n",label);}};
 DrawCallKey a{},b{};
 auto* ar=reinterpret_cast<unsigned char*>(&a);auto* br=reinterpret_cast<unsigned char*>(&b);
 for(std::size_t i=0;i<fields;++i)ar[i]=br[i]=static_cast<unsigned char>(i*17U+3U);
 std::memset(ar+fields,0x55,sizeof(a)-fields);std::memset(br+fields,0xAA,sizeof(b)-fields);
 check(a.hash()==b.hash(),"logical key hash ignores tail padding");
 check(a==a && a==b && b==a,"logical equality ignores tail padding and is reflexive");
 const auto hash=a.hash();
 for(std::size_t i=0;i<fields;++i){br[i]^=1U;check(!(a==b),"logical differences compare unequal");check(hash!=b.hash(),"each field participates in key");br[i]^=1U;}
 RT64::PresetDrawCallLibrary empty;RT64::PresetMaterialLibrary materials;
 for(unsigned i=0;i<256;++i){std::memset(br+fields,static_cast<int>(i),sizeof(b)-fields);auto range=empty.findMaterialsInCache(b,materials);check(range.first==range.second,"empty library has no material");}
 check(empty.cachedKeyMap.empty(),"empty material library does not accumulate negative cache keys");
 RT64::PresetDrawCallLibrary populated;RT64::PresetDrawCall unused{};unused.enabled=false;populated.presetMap["disabled"]=unused;
 for(unsigned i=0;i<256;++i){std::memset(br+fields,static_cast<int>(i),sizeof(b)-fields);populated.findMaterialsInCache(b,materials);}
 check(populated.cachedKeyMap.size()==1,"one logical draw key occupies one cache entry");
 std::printf("draw key regression failures: %u\n",failed);return failed?1:0;
}
