/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(void)
{
    host_tracks_init();uint32_t rng_before=rng_state;
    for(unsigned i=0;i<1000;i++)for(unsigned p=0;p<NPART;p++)drift_tick(&trk[p]);
    assert(rng_state==rng_before);
    for(unsigned p=0;p<NPART;p++)assert(!drift[p].phase && !drift[p].value);
    int16_t patch[P_COUNT];memcpy(patch,trk[0].p,sizeof patch);
    drift_p[0][0]=1;drift_p[0][1]=0;drift_p[0][2]=127;
    int32_t last=0;unsigned wraps=0;uint32_t old=0;
    for(unsigned b=0;b<FS*60u/CTL;b++) {
        drift_tick(&trk[0]);
        assert(abs(drift_mod(&trk[0],0))<=16256);
        assert(abs(drift_mod(&trk[0],0)-last)<=40);last=drift_mod(&trk[0],0);
        assert(!drift_mod(&trk[0],1) && !drift_mod(&trk[0],2));
        wraps+=drift[0].phase<old;old=drift[0].phase;
    }
    assert(wraps>=11 && wraps<=12 && last!=0);
    assert(!memcmp(patch,trk[0].p,sizeof patch) && rng_state==rng_before);
    assert(!drift[1].phase && !drift[2].phase && !drift_mod(TDRUM,0));
    /* Target changes retire the old destination before the new one rises. */
    drift_p[0][3]=2;unsigned transitioned=0;
    for(unsigned i=0;i<1200;i++) {
        drift_tick(&trk[0]);
        if(drift[0].target==2)transitioned=1;
        assert(!drift_mod(&trk[0],0) || !drift_mod(&trk[0],2));
        assert(live_pan(&trk[0])>=-64 && live_pan(&trk[0])<=63);
    }
    assert(transitioned && drift_mod(&trk[0],2));
    /* Combined destinations reuse the exact same bounded motion, not extra
     * generators, and never leak into an unselected destination. */
    static const uint8_t masks[]={1,2,4,3,5,6,7};
    for(unsigned choice=0;choice<NELEM(masks);choice++) {
        drift[0].target=(uint8_t)choice;drift[0].value=-8192;
        for(unsigned destination=0;destination<3;destination++)
            assert(drift_mod(&trk[0],destination)==(masks[choice]&(1u<<destination)?-8192:0));
    }
    drift[0].target=2; /* Resume the fade from the previously selected PAN. */
    drift_p[0][0]=0;for(unsigned i=0;i<600;i++)drift_tick(&trk[0]);
    assert(!drift[0].depth && !drift[0].value);
    /* The slowest choice advances 32 times more slowly, without altering patches. */
    memset(drift,0,sizeof drift);drift_p[0][0]=1;drift_p[0][1]=5;
    for(unsigned i=0;i<1000;i++)drift_tick(&trk[0]);
    assert(drift[0].phase==(623305u>>5)*1000u);
    /* Prove that the drift reaches the real oscillator/filter render without
     * rewriting its patch, rather than testing a disconnected control source. */
    host_tracks_init();track_t *t=&trk[0];host_preset(t,0,0);
    t->p[P_E0]=0;t->p[P_E4]=70;t->p[P_ED_FLT]=0;t->p[P_ED_PIT]=0;
    t->p[P_SUS]=127;
    trk_note_on(t,60,100);track_t saved=*t;uint32_t saved_rng=rng_state;
    int32_t dry[CTL],wet[CTL];memset(drift,0,sizeof drift);drift_p[0][0]=0;
    track_render(t,dry,CTL);*t=saved;rng_state=saved_rng;
    drift_p[0][0]=1;drift_p[0][3]=0;drift[0].target=0;drift[0].rng=1;
    drift[0].from=drift[0].to=16384;drift[0].depth=16256;
    track_render(t,wet,CTL);
    assert(memcmp(dry,wet,sizeof dry) && !memcmp(saved.p,t->p,sizeof t->p));
    /* LOFI: bipolar DRIFT equals an actual tone adjustment; legacy negative
     * envelope/LFO response remains byte-identical when DRIFT is off. */
    host_preset(t,3,0);t->p[P_E7]=64;
    vmod_t m={0};m.pitch16=60*16;m.amp0=m.amp1=32767;
    m.shape=64<<8;
    voice_t seed={0};seed.note=60;lofi_note_on(t,&seed);
    int32_t negative[CTL];
    for(int sign=-1;sign<=1;sign+=2) {
        voice_t a=seed,b=seed;
        memset(dry,0,sizeof dry);memset(wet,0,sizeof wet);
        drift[0].target=0;drift[0].value=sign*8192;m.cutoff=sign*8192;
        lofi_render(t,&a,dry,CTL,&m);
        drift[0].value=0;m.cutoff=0;t->p[P_E7]=64+sign*32;
        lofi_render(t,&b,wet,CTL,&m);
        assert(!memcmp(dry,wet,sizeof dry));
        if(sign<0)memcpy(negative,dry,sizeof dry);
        else assert(memcmp(negative,dry,sizeof dry));
        t->p[P_E7]=64;
    }
    voice_t a=seed,b=seed;m.cutoff=-8192;
    memset(dry,0,sizeof dry);memset(wet,0,sizeof wet);
    lofi_render(t,&a,dry,CTL,&m);m.cutoff=0;
    lofi_render(t,&b,wet,CTL,&m);assert(!memcmp(dry,wet,sizeof dry));
    /* Lowest tone plus maximum negative drift cannot invert the lowpass. */
    t->p[P_E7]=0;drift[0].value=-16256;m.cutoff=-16256;a=seed;memset(dry,0,sizeof dry);
    lofi_render(t,&a,dry,CTL,&m);assert(a.s[4]>=-65535 && a.s[4]<=65535);
    puts("ambient drift: continuity, bounds, 5–160 s cycles, isolated RNG/track/patch, target fade and OFF PASS");
    return 0;
}
