/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen original decoder oracle, including defensive index cap and loop/held
 * state semantics, rather than checking the new magnitude in isolation. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
DSP_INLINE int32_t old_next(const smp_zone_t *z, voice_t *v, int loop)
{
    uint32_t pos = v->ph[0], b = SMP_DATA[z->off + (pos >> 1)];
    uint32_t code = (pos & 1u) ? (b >> 4) : (b & 15u);
    int32_t step = IMA_STEP[(uint32_t)v->s[1] <= 88u ? v->s[1] : 88], vd = step >> 3;
    if (code & 4u)
        vd += step;
    if (code & 2u)
        vd += step >> 1;
    if (code & 1u)
        vd += step >> 2;
    v->s[0] = clamp(v->s[0] + ((code & 8u) ? -vd : vd), -32768, 32767);
    v->s[1] = clamp(v->s[1] + IMA_IDX[code & 7u], 0, 88);
    pos++;
    if (pos > z->le && z->looped && loop && (z->looped != SMP_LOOP_HELD || voice_engine_held(v))) {   /* (HELD, let go: the tail) */
        pos = z->ls;
        v->s[0] = z->pred;
        v->s[1] = z->idx;
    }
    v->ph[0] = pos;
    return v->s[0];
}

int main(void) {
 host_tracks_init();unsigned cases=0;
 for(unsigned zi=0;zi<NELEM(SMP_ZONES);zi++) {
  const smp_zone_t *z=&SMP_ZONES[zi];uint32_t pp[]={0,z->ls,z->le,z->le?z->le-1:0,z->n-1};
  for(unsigned j=0;j<NELEM(pp);j++)for(unsigned idx=0;idx<90;idx++)for(unsigned loop=0;loop<2;loop++)for(unsigned gate=0;gate<2;gate++)for(unsigned pr=0;pr<3;pr++) {
   voice_t x={0},y;x.ph[0]=pp[j];x.s[0]=pr==0?-32768:pr==1?0:32767;x.s[1]=idx;x.gate=gate;y=x;
   int32_t aa=old_next(z,&x,loop),bb=sample_next_inline(z,&y,loop);
   assert(aa==bb&&!memcmp(&x,&y,sizeof x));cases++;
  }
 }
 printf("SAMPLE/drum decoder: %u full-state boundary cases PASS\n",cases);return 0;
}
