/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
int main(void) {
    host_tracks_init();voice_expand=4;cpu1_units=-1;
    static const uint32_t engines[]={8,5,3},notes[]={3,4,5};
    for(uint32_t p=0;p<NPART;p++) {
        host_preset(&trk[p],engines[p],0);trk[p].p[P_VOICE]=V_POLY;trk[p].p[P_AMODE]=0;
        trk[p].p[P_LSYNC]=0;trk[p].p[P_LRATE]=0;trk[p].lfo_ph=0;
    }
    assert(units_split()==12); /* historical idle tie preference */
    for(uint32_t p=0;p<NPART;p++)for(uint32_t n=0;n<notes[p];n++)trk_note_on(&trk[p],48+n*3,100);
    int32_t out[CTL*2];mix_block(out,CTL);
    assert(units_split()==1); /* GRAIN on worker; lighter core 0 takes drums. */
    for(uint32_t i=0;i<NVOICE;i++)trk[0].v[i].gate=0;
    assert(units_split()==1); /* release tails still consume render time */
    for(uint32_t i=0;i<NVOICE;i++)trk[0].v[i].active=0;
    mix_block(out,CTL);
    assert(units_split()==2); /* VOICE alone on worker after GRAIN finishes */
    for(uint32_t mask=0;mask<16;mask++) {cpu1_units=(int32_t)mask;assert(units_split()==mask);}
    cpu1_units=-1;trk[1].lfo_ph=UINT32_MAX;
    assert(units_split()==12); /* skip VOICE wrap, offload eligible LOFI instead */
    for(uint32_t p=0;p<NPART;p++) {
        host_preset(&trk[p],5,0);trk[p].p[P_VOICE]=V_POLY;trk[p].p[P_AMODE]=0;
        trk[p].lfo_ph=0;trk[p].p[P_LSYNC]=trk[p].p[P_LRATE]=0;trk_note_on(&trk[p],60,100);
    }
    assert(units_split()==8); /* shared formant RNG stays on one core */
    host_tracks_init();host_preset(&trk[2],ENGI_GRAIN,2);
    trk[2].p[P_VOICE]=V_POLY;trk[2].p[P_LSYNC]=trk[2].p[P_LRATE]=0;
    for(uint32_t n=60;n<=67;n+=3)trk_note_on(&trk[2],n,100);
    mix_block(out,CTL);cpu1_units=-1;
    assert(units_split()==4); /* Reported Flute Dust + drums: avoid mask 12. */
    for(uint32_t type=0;type<NELEM(N_LOFI);type++) {
        song.g[G_LTYPE]=(int16_t)type;mix_block(out,CTL);
        trk[2].lfo_ph=0;assert(units_split()==4);
    }
    puts("auto split: GRAIN/VOICE/LOFI workload, tails/idle, manual masks, LFO and FORMANT exclusions PASS");
}
