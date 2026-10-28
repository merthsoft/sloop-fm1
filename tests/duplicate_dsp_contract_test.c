/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
DSP_INLINE void oracle_coef(tsvf_t *c, int32_t cut, int32_t reso)   /* cut: 0..127 << 8 */
{
    int32_t i, g, den, k = 8192 - reso * 7600 / 127;   /* damping 2.0 .. ~0.15 (Q12) */
    cut = clamp(cut, 0, 127 << 8);
    i = cut >> 8;
    g = SVF_G[i];
    if (i < 127)                                       /* between table points: sweeps without 128 steps */
        g += ((SVF_G[i + 1] - g) * (cut & 255)) >> 8;
    den = 4096 + ((g * (g + k)) >> 12);
    c->a1 = (int32_t)((4096u << 13) / (uint32_t)den);
    c->a2 = (c->a1 * g) >> 12;
    c->a3 = (c->a2 * g) >> 12;
}


int main(void){
 host_tracks_init();
 for(int res=0;res<128;res++)for(int cut=-256;cut<=32768;cut++){
  tsvf_t a,b;tsvf_coef(&a,cut,res);oracle_coef(&b,cut,res);assert(!memcmp(&a,&b,sizeof a));
 }
 unsigned cases=0;
 for(unsigned zi=0;zi<sizeof(SMP_ZONES)/sizeof(SMP_ZONES[0]);zi++){
  const smp_zone_t *z=&SMP_ZONES[zi];
  for(unsigned loop=0;loop<2;loop++)for(unsigned gate=0;gate<2;gate++)for(unsigned idx=0;idx<90;idx++){
   voice_t a={0},b;a.gate=gate;a.s[0]=(int)idx*721-32768;a.s[1]=idx;a.ph[0]=idx%z->n;b=a;
   int x=sample_next_inline(z,&a,loop),y=sample_next(z,&b,loop);assert(x==y && !memcmp(&a,&b,sizeof a));cases++;
  }
 }
 printf("4227200 coefficient cases, %u decoder init/state cases PASS\n",cases);return 0;
}
