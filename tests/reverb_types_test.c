/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual impulse responses: character, decay, stereo, bounds and transition lifecycle. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#define IR_N (FS*8u)
static double energy[IR_N],brightness;
static void reset(int model,int size,int damp) {
 host_tracks_init();punch_clear();rev_frz=0;
 memset(&fx,0,sizeof fx);memset(rev_line,0,sizeof rev_line);
 memset(&rev_storage,0,sizeof rev_storage);memset(&spring,0,sizeof spring);
 reverb_model.cur=model;reverb_model.gain=32768;
 song.g[G_RMODEL]=model;song.g[G_RSIZE]=size;song.g[G_RDAMP]=damp;
}
static double impulse(int model,int size,int damp,uint64_t *hash,double *corr) {
 reset(model,size,damp);int32_t z[CTL]={0},in[CTL],l[CTL],r[CTL];
 double ll=0,rr=0,lr=0,high=0;int32_t pl=0,pr=0;uint64_t h=0xcbf29ce484222325ull;
 for(unsigned t=0;t<IR_N;t+=CTL) {
  memset(in,0,sizeof in);if(!t)in[0]=120000;
  fx_buses(z,z,in,l,r,CTL);
  for(unsigned i=0;i<CTL&&t+i<IR_N;i++) {
   assert(abs(l[i])<(1<<22)&&abs(r[i])<(1<<22));
   energy[t+i]=(double)l[i]*l[i]+(double)r[i]*r[i];
   h=(h^(uint32_t)l[i])*1099511628211ull;h=(h^(uint32_t)r[i])*1099511628211ull;
   if(t+i>FS/20 && t+i<FS) {high+=(double)(l[i]-pl)*(l[i]-pl)+(double)(r[i]-pr)*(r[i]-pr);ll+=(double)l[i]*l[i];rr+=(double)r[i]*r[i];lr+=(double)l[i]*r[i];}pl=l[i];pr=r[i];
  }
 }
 brightness=high;
 double floor=0;for(unsigned t=IR_N-FS;t<IR_N;t++)floor+=energy[t]/FS;
 for(unsigned t=0;t<IR_N;t++)energy[t]=fmax(0,energy[t]-floor*1.25);
 for(int t=IR_N-2;t>=0;t--)energy[t]+=energy[t+1];
 assert(energy[0]>1e6 && energy[IR_N-1]<energy[0]*1e-6);
 unsigned a=0,b=0;
 for(unsigned t=0;t<IR_N;t++){if(!a&&energy[t]<energy[0]*.316228)a=t;if(!b&&energy[t]<energy[0]*.00316228)b=t;}
 assert(b>a);*hash=h;*corr=lr/sqrt(ll*rr);return 3.*(b-a)/FS;
}
static void lifecycle(int model) {
 reset(model,90,45);int32_t z[CTL]={0},in[CTL],l[CTL],r[CTL];
 for(int b=0;b<1000;b++) {
  for(unsigned i=0;i<CTL;i++)in[i]=(b+i)&1?(1<<22):-(1<<22);
  fx_buses(z,z,in,l,r,CTL);
  for(unsigned i=0;i<CTL;i++)assert(abs(l[i])<(1<<22)&&abs(r[i])<(1<<22));
 }
 rev_frz=1;song.g[G_RMODEL]=(model+1)%5;
 for(int b=0;b<3000;b++) {
  fx_buses(z,z,z,l,r,CTL);assert(reverb_model.cur==model&&reverb_model.gain==32768);
  for(unsigned i=0;i<CTL;i++)assert(abs(l[i])<(1<<22)&&abs(r[i])<(1<<22));
 }
 rev_frz=0;
 for(int b=0;b<10;b++)fx_buses(z,z,z,l,r,CTL);
 assert(reverb_model.cur==(model+1)%5&&reverb_model.gain==32768);
 for(unsigned i=0;i<CTL;i++)assert(l[i]==0&&r[i]==0);
}
int main(void) {
 setvbuf(stdout,0,_IOLBF,0);uint64_t hashes[5];double decay[5];
 for(int m=0;m<5;m++) {
  uint64_t h;double c,short_rt=impulse(m,20,20,&h,&c);
  double long_rt=impulse(m,100,20,&hashes[m],&c);decay[m]=long_rt;
  printf("model%d: RT60 fit %.3f -> %.3f sec, stereo correlation %.3f\n",m,short_rt,long_rt,c);
  assert(long_rt>short_rt*1.15 && fabs(c)<.95);
  double bright=brightness,damped=impulse(m,100,127,&h,&c);
  printf("model%d: damped RT60 fit %.3f sec, HF energy %.3e -> %.3e\n",m,damped,bright,brightness);
  assert(brightness<bright*.8 && damped<long_rt*1.1);
  for(int j=0;j<m;j++)assert(hashes[j]!=hashes[m]);
  lifecycle(m);
 }
 assert(decay[2]<decay[3]&&decay[2]<decay[4]);
 puts("reverb types: distinct stereo/decay, damping, overload, FREEZE and cleared switching PASS");return 0;
}
