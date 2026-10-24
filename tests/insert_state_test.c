/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static uint32_t rnd=982451653u;
static uint32_t next(void){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;return rnd;}
static uint64_t hash=1469598103934665603ull;
static void sample_hash(int32_t v){for(unsigned i=0;i<4;i++)hash=(hash^((uint32_t)v>>(8*i)&255u))*1099511628211ull;}
int main(void){
 host_tracks_init();
 int16_t p[P_COUNT]={0};int32_t b[CTL];ins_t s={0};
 for(unsigned k=0;k<160000;k++){
  if(k%173==0){p[P_ITYPE]=next()%IT_N;p[P_IMIX]=next()%128;p[P_IA]=next()%128;p[P_IB]=next()%128;p[P_IC]=next()%128;}
  if(k%719==0)p[P_IMIX]=0;
  unsigned n=1+next()%CTL;
  for(unsigned i=0;i<n;i++)b[i]=(int32_t)(next()%120001)-60000;
  int ret=ins_run(&s,p,b,n);
  sample_hash(ret);for(unsigned i=0;i<n;i++)sample_hash(b[i]);
 }
 assert(hash==0x523b86060b55a214ull);
 puts("insert state: 160000 type/mix transitions match pre-union output oracle PASS");
 return 0;
}
