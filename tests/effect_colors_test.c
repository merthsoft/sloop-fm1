/* SPDX-License-Identifier: GPL-3.0-only */
/* Frequency probes exercise real LOFI and delay paths, not just nonzero output. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static double radio_energy(double hz,int band) {
 host_tracks_init();punch_clear();memset(&vinyl,0,sizeof vinyl);vinyl.mode=VINYL_MODE_RADIO;
 song.playing=1;song.g[G_VTONE]=band;song.g[G_VWOW]=song.g[G_VCRACKLE]=0;
 double e=0;int32_t l[CTL],r[CTL];
 for(uint32_t b=0;b<1600;b++) {
  for(uint32_t i=0;i<CTL;i++)l[i]=r[i]=(int32_t)(10000*sin((b*CTL+i)*hz*6.283185307/FS));
  vinyl_process_mode(l,r,CTL,VINYL_MODE_RADIO);
  if(b>800)for(uint32_t i=0;i<CTL;i++)e+=(double)l[i]*l[i];
 }
 return e;
}
static double dub_energy(double hz,int tone,int mode) {
 host_tracks_init();memset(&fx,0,sizeof fx);memset(dly_buf,0,sizeof dly_buf);memset(&delay_age,0,sizeof delay_age);
 song.g[G_BPM]=120;song.g[G_DTIME]=1;song.g[G_DTYPE]=mode;song.g[G_DCOLOR]=tone;
 song.g[G_DDRIVE]=0;song.g[G_DFDBK]=0;song.g[G_DMIX]=127;
 uint32_t dl=delay_samples();double e=0;
 for(uint32_t t=0;t<dl+FS/4;t+=CTL) {
  int32_t z[CTL]={0},in[CTL],l[CTL],r[CTL];
  for(uint32_t i=0;i<CTL;i++)in[i]=(int32_t)(14000*sin((t+i)*hz*6.283185307/FS));
  fx_buses(z,in,z,l,r,CTL);
  if(t>dl+FS/10)for(uint32_t i=0;i<CTL;i++)e+=(double)l[i]*l[i];
 }
 return e;
}
int main(void) {
 double bass=radio_energy(30,127)/radio_energy(30,0);
 double treble=radio_energy(8000,127)/radio_energy(8000,0);
 double mid=radio_energy(1000,127)/radio_energy(1000,0);
 printf("Radio: band/dry energy bass %.4f mid %.4f treble %.4f\n",bass,mid,treble);
 assert(bass<.25&&treble<.25&&mid>.03&&mid>bass&&mid>treble);
 double dark=dub_energy(6000,0,3),bright=dub_energy(6000,127,3),tape=dub_energy(6000,0,1);
 printf("Dub first repeat: dark %.3e bright %.3e Tape %.3e\n",dark,bright,tape);
 assert(bright>dark*10&&dark<tape*.1&&bright<tape*.6);
 puts("effect colors: Radio receiver band and Dub first-repeat tone control PASS");return 0;
}
