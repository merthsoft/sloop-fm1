/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise actual oscillator rendering, including knob changes under a held note. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static void render(track_t *t,voice_t *v,uint32_t blocks)
{
    vmod_t m={0};m.pitch16=60*16;m.amp0=m.amp1=16000;
    for(uint32_t b=0;b<blocks;b++) {
        int32_t out[CTL]={0};
        lofi_render(t,v,out,CTL,&m);
        assert(v->s[3]>=0 && v->s[3]<=127*768);
        for(uint32_t i=0;i<CTL;i++)assert(out[i]>=-32768 && out[i]<=32767);
    }
}
int main(void)
{
    host_tracks_init();track_t *t=&trk[0];host_preset(t,3,0);
    memset(drift_p,0,sizeof drift_p);memset(drift,0,sizeof drift);
    for(int32_t amount=0;amount<128;amount++) {
        voice_t v={0};v.note=60;t->p[P_E4]=(int16_t)amount;
        lofi_note_on(t,&v);render(t,&v,2048);
        assert(v.s[3]==amount*768); /* Every nonzero value works, including 1..7. */
        int32_t settled=v.s[3];render(t,&v,8192);
        assert(v.s[3]==settled); /* A long hold cannot continue falling. */
    }
    voice_t v={0};v.note=60;t->p[P_E4]=127;lofi_note_on(t,&v);
    render(t,&v,128);
    assert(v.s[3]>60000 && v.s[3]<65000); /* ~63% after 93 ms. */
    render(t,&v,300);assert(v.s[3]>94000);
    t->p[P_E4]=8;int32_t before=v.s[3];render(t,&v,1);
    assert(v.s[3]<before && before-v.s[3]<800); /* No jump on live edits. */
    render(t,&v,2048);assert((v.s[3]>>8)==24); /* 6%: 1.5 semitones. */
    t->p[P_E4]=0;render(t,&v,2048);assert(!(v.s[3]>>8));
    t->p[P_E4]=127;render(t,&v,2048);assert((v.s[3]>>8)==381);
    lofi_note_on(t,&v);assert(!v.s[3]); /* New attacks restart the sweep. */
    puts("LOFI SWP: all 128 depths, no dead zone/runaway, attack timing, live edits, zero and retrigger PASS");
}
