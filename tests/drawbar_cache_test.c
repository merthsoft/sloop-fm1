/* SPDX-License-Identifier: GPL-3.0-only */
/* Frozen uncached control path: compare all audible/rotor state while the new
 * path caches registration and continues live rotor movement. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <stddef.h>
static void reference_block(track_t *t, drw_trk_t *T)
{
    const int16_t *p = t->p;
    uint32_t k, perc = (uint32_t)p[P_E4] % 7u, rot = (uint32_t)p[P_E7] % 3u;
    int32_t s = 0, lv[DRW_NP];
    uint32_t norm;
    for (k = 0; k < DRW_NP; k++) {
        lv[k] = perc && k == DRW_NP - 1u ? 0 : DRW_LV[drw_level(p, k)];   /* PERC on: the 1' bar is off */
        s += lv[k];
    }
    /* sum of the gains = 2.4 s / (s + 1.5) (s in units of a full bar): one bar 0.96, three 1.6,
     * all nine 2.06 */
    norm = (uint32_t)(2u * 39322u * 32768u) / (uint32_t)(s + 49152);
    if (perc == 1u || perc == 2u || perc == 5u || perc == 6u)
        norm = (norm * 23170u) >> 15;                   /* normal percussion: the bars -3 dB */
    for (k = 0; k < DRW_NP; k++)
        T->g[k] = (int32_t)(((uint32_t)lv[k] * norm) >> 15);
    T->pk = perc ? (int32_t)(3u + ((perc - 1u) & 1u)) : DRW_NP;   /* 2ND: 4', 3RD: 2 2/3' */
    T->plev = perc == 3u || perc == 4u ? 12000 : 24000;
    T->pdec = perc >= 5u ? 65457u : 65219u;             /* tau 0.6 s / 0.15 s */
    T->clk = p[P_E5] * 180;
    if (p[P_E6]) {                                      /* y = 6 / (2 + g) tanh(g x / 2) */
        T->dg = 256 + p[P_E6] * 6;                      /* 1 .. 4 */
        T->dcomp = (6 * 256 * 256) / (512 + T->dg);
    } else {
        T->dg = 0;
    }
    /* rotors: each speed eases to its target (high rotor tau ~0.37 s, low ~1.5 s) */
    for (k = 0; k < 2u; k++) {
        int32_t sp, c, sn, dep;
        T->rsp[k] += (DRW_ROT_SPD[k][rot] - T->rsp[k]) >> (k ? 11 : 9);
        sp = T->rsp[k];
        T->rph[k] += (uint32_t)sp;
        sn = sine_i(T->rph[k]);
        c = sine_i(T->rph[k] + 0x40000000u);
        dep = k ? 7000 : 13000;                         /* AM depth: low rotor 21 %, high 40 % */
        T->am[k] = 32767 - mulq15(dep, (32767 - c) >> 1);
        T->fm[k] = (sn * (sp >> (k ? 17 : 16))) >> 15;  /* Doppler: +-0.5 % fast (high), +-0.2 % (low) */
    }
    T->rwet += ((rot ? 32767 : 0) - T->rwet) >> 5;      /* ~25 ms in or out */
    for (k = 0; k < 2u; k++) {
        T->am[k] = 32767 - mulq15(T->rwet, 32767 - T->am[k]);
        T->fm[k] = mulq15(T->fm[k], T->rwet);
    }
}

int main(void) {
    drw_trk_t ref={0}; uint32_t seed=17, steps=0;
    host_tracks_init(); host_preset(&trk[0],ENGI_DRAWBAR,0);
    for(uint32_t i=0;i<200000;i++) {
        if(i%1024==0) { memset(eng_arena_own,0,sizeof eng_arena_own); memset(&ref,0,sizeof ref); }
        if(i%13==0) for(uint32_t k=0;k<8;k++) {
            seed=seed*1664525u+1013904223u;
            static const int lo[]={0,-8,-8,-8,0,0,0,0};
            static const unsigned span[]={16,17,17,17,7,128,128,3};
            trk[0].p[P_E0+k]=lo[k]+(int)(seed%span[k]);
        }
        if(i%13 && !(i%7)) trk[0].p[P_E7]=(int16_t)((i/7)%3); /* Rotor-only cache hits. */
        reference_block(&trk[0],&ref);drawbar_block(&trk[0]);
        drw_trk_t *got=&drw_of(&trk[0])->t;
        size_t off=offsetof(drw_trk_t,g);
        if(memcmp((char*)&ref+off,(char*)got+off,sizeof ref-off)) {printf("mismatch %u\n",i);return 1;}
        steps++;
    }
    printf("DRAWBAR original control and complete rotor/output state match: %u blocks; state %zu arena %zu FM6 %zu DRAWBAR %zu\n",steps,sizeof(drw_trk_t),sizeof(eng_arena_t),sizeof(fm6_part_t),sizeof(drw_part_t));return 0;
}
