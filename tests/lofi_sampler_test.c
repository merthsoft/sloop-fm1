/* SPDX-License-Identifier: GPL-3.0-only */
/* Verify audible mechanisms, not merely different hashes. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void prep(int rate,int bits,int drive) {
 host_tracks_init();punch_clear();memset(&vinyl,0,sizeof vinyl);vinyl.mode=VINYL_MODE_SAMPLER;
 song.playing=1;song.g[G_VWOW]=rate;song.g[G_VTONE]=bits;song.g[G_VCRACKLE]=drive;
}
static double error_at(unsigned mask,int amount) {
 prep(mask&1?amount:0,mask&2?amount:0,mask&4?amount:0);
 int32_t l[CTL],r[CTL],dry[CTL];double error=0;
 for(unsigned b=0;b<400;b++) {
  for(unsigned i=0;i<CTL;i++) {unsigned n=b*CTL+i;l[i]=dry[i]=(int32_t)(90000*sin(n*.13)+25000*sin(n*.71));r[i]=0;}
  vinyl_process_mode(l,r,CTL,VINYL_MODE_SAMPLER);
  if(b>200)for(unsigned i=0;i<CTL;i++){double e=l[i]-dry[i];error+=e*e;assert(r[i]==0);}
 }
 return sqrt(error/(199*CTL));
}
int main(void) {
 for(unsigned mask=1;mask<=7;mask++) {
  double zero=error_at(mask,0),low=error_at(mask,8),middle=error_at(mask,32),full=error_at(mask,127);
  printf("sampler controls %u: error RMS zero/low/mid/full %.1f %.1f %.1f %.1f\n",mask,zero,low,middle,full);
  assert(zero==0 && full>100 && low<full*.1 && middle<full*.6 && low<=middle);
 }
 /* Twenty-percent DRIVE should stay mild even on a loud internal bus. */
 double drive20=error_at(4,25),drive50=error_at(4,64),drive100=error_at(4,127);
 printf("sampler drive 20/50/100%% error RMS %.1f %.1f %.1f\n",drive20,drive50,drive100);
 assert(drive20<3 && drive20<drive50*.1 && drive50<drive100*.5);
 int32_t l[CTL],r[CTL];unsigned changes=0,frames=0;int32_t last=0;
 prep(127,0,0);
 for(unsigned b=0;b<200;b++) {
  for(unsigned i=0;i<CTL;i++){l[i]=((b*CTL+i)*17)%20000-10000;r[i]=-l[i];}
  vinyl_process_mode(l,r,CTL,VINYL_MODE_SAMPLER);
  if(b>100)for(unsigned i=0;i<CTL;i++){changes+=l[i]!=last;last=l[i];frames++;assert(r[i]==-l[i]);}
 }
 assert(changes>frames/40 && changes<frames/24); /* 32-frame sample hold */
 prep(0,127,0);
 for(unsigned b=0;b<200;b++) {
  for(unsigned i=0;i<CTL;i++){l[i]=(i*713)%18000-9000;r[i]=-l[i];}
  vinyl_process_mode(l,r,CTL,VINYL_MODE_SAMPLER);
  if(b>100)for(unsigned i=0;i<CTL;i++)assert(l[i]%4096==0 && r[i]==-l[i]);
 }
 /* Full drive remains bounded and cannot turn silence into noise. */
 prep(64,64,127);double power=0;
 for(unsigned b=0;b<300;b++) {
  for(unsigned i=0;i<CTL;i++){l[i]=(int32_t)(12000*sin((b*CTL+i)*.13));r[i]=0;}
  vinyl_process_mode(l,r,CTL,VINYL_MODE_SAMPLER);
  if(b>100)for(unsigned i=0;i<CTL;i++){power+=(double)l[i]*l[i];assert(r[i]==0&&abs(l[i])<=131072);}
 }
 assert(sqrt(power/(199*CTL))<40000);
 puts("sampler: real rate hold, signed quantization, stereo isolation and compensated drive PASS");return 0;
}
