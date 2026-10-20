/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
static int impulse(int ms,int size) {
 host_tracks_init();memset(&spring,0,sizeof spring);memset(rev_line,0,sizeof rev_line);
 memset(&rev_storage,0,sizeof rev_storage);rev_frz=0;
 song.g[G_RTIME]=ms;song.g[G_RSIZE]=size;song.g[G_RDAMP]=0;
 int first=-1;
 for(uint32_t t=0;t<FS/5;t++) {
  if(!(t%CTL))spring_begin();
  int32_t l,r;spring_sample(t?0:120000,&l,&r);
  assert(abs(l)<(1<<22) && abs(r)<(1<<22));
  if(first<0 && abs(l)+abs(r)>4)first=(int)t;
 }
 assert(first>=0);return first;
}
int main(void) {
 for(int size=0;size<=127;size+=127) {
  int a=impulse(20,size),b=impulse(80,size);
  printf("SPRING size%d: first reflection at20ms=%d,80ms=%d samples\n",size,a,b);
  assert(abs(a-(int)FS/100)<80 && abs(b-(int)FS*4/100)<80 && b>a*3);
  double rt[2];
  for(int k=0;k<2;k++) {
   impulse(k?80:20,size);
   rt[k]=-(double)(spring.size>>8)/FS*log(1000)/log(spring.gain/32768.);
  }
  assert(rt[1]/rt[0]>.95 && rt[1]/rt[0]<1.05);
  printf("SPRING nominal decay at fixed SIZE: %.3f / %.3f sec\n",rt[0],rt[1]);
 }
 impulse(50,90);int len=spring.size;rev_frz=1;song.g[G_RTIME]=80;
 for(int b=0;b<100;b++)spring_begin();assert(spring.size==len);
 rev_frz=0;for(int b=0;b<3000;b++)spring_begin();assert(spring.size>len);
 assert((spring.size>>8)==FS*80/1000);
 puts("SPRING TIME: independent reflection spacing/nominal decay, bounded impulses and FREEZE preservation PASS");
 return 0;
}
