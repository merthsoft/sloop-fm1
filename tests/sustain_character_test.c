/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

typedef struct { uint64_t held, tail, energy, edge; } result_t;
static result_t render(uint32_t type,uint32_t damp,uint32_t amount,const char *path)
{
    memset(trk,0,sizeof trk);host_tracks_init();rng_state=0x1234567u;
    track_t *t=&trk[0];host_preset(t,0,0);
    t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=0;t->p[P_ATK]=t->p[P_DEC]=t->p[P_REL]=0;
    t->p[P_SUS]=127;t->p[P_ED_FLT]=0;t->p[P_LD_FLT]=0;
    sustain_p[0][0]=(int16_t)type;sustain_p[0][1]=2;
    sustain_p[0][2]=(int16_t)damp;sustain_p[0][3]=(int16_t)amount;
    FILE *f=path?fopen(path,"wb"):0;
    uint32_t held_blocks=FS/CTL/4,tail_blocks=FS/CTL;
    if(f)wav_hdr(f,(held_blocks+tail_blocks)*CTL);
    result_t r={1469598103934665603ull,1469598103934665603ull,0,0};
    int32_t out[CTL],prev=0;trk_note_on(t,84,100);
    for(uint32_t block=0;block<held_blocks+tail_blocks;block++) {
        if(block==held_blocks)trk_note_off(t,84);
        track_render(t,out,CTL);
        for(uint32_t i=0;i<CTL;i++) {
            uint64_t *h=block<held_blocks?&r.held:&r.tail;
            *h=(*h^(uint32_t)out[i])*1099511628211ull;
            if(block>=held_blocks) {
                int64_t x=out[i],d=x-prev;r.energy+=(uint64_t)(x*x);r.edge+=(uint64_t)(d*d);
            }
            prev=out[i];if(f)wav_put(f,out[i],out[i]);
        }
    }
    if(f)fclose(f);
    return r;
}
int main(int argc,char **argv)
{
    const char *dir=argc>1?argv[1]:"build/host";
    result_t dry=render(0,0,127,0),pedal=render(1,0,127,0),modes[5];
    assert(render(4,127,0,0).tail==dry.tail); /* zero amount is ordinary release */
    for(uint32_t type=1;type<5;type++) {
        char path[512];snprintf(path,sizeof path,"%s/sustain-%u.wav",dir,type);
        modes[type]=render(type,32,127,path);
        assert(modes[type].held==dry.held && modes[type].energy>dry.energy*4);
        for(uint32_t prior=1;prior<type;prior++)assert(modes[type].tail!=modes[prior].tail);
        result_t low=render(type,32,25,0);
        assert(low.energy<modes[type].energy/4);
        for(uint32_t damp=0;damp<=127;damp+=127)assert(render(type,damp,127,0).held==dry.held);
    }
    result_t dark=render(1,127,127,0);
    assert(dark.edge< pedal.edge && dark.energy<pedal.energy);
    assert(modes[2].energy<modes[3].energy); /* treble piano fades faster than strings */
    /* Character processing is engine-neutral; no engine state slots are borrowed. */
    for(uint32_t e=0;e<NENGINES;e++)for(uint32_t type=1;type<5;type++) {
        memset(trk,0,sizeof trk);host_tracks_init();host_preset(&trk[0],e,0);
        trk[0].p[P_VOICE]=V_POLY;trk[0].p[P_AMODE]=0;
        sustain_p[0][0]=(int16_t)type;sustain_p[0][1]=2;sustain_p[0][2]=127;sustain_p[0][3]=127;
        int32_t out[CTL];trk_note_on(&trk[0],60,100);
        for(uint32_t b=0;b<200;b++) {
            if(b==30)trk_note_off(&trk[0],60);
            track_render(&trk[0],out,CTL);
        }
        trk_all_off(&trk[0]);
        for(uint32_t i=0;i<NVOICE;i++)assert(!trk[0].v[i].sustained);
    }
    /* Decay contracts: treble piano faster, strings unchanged; HOLD stays indefinite. */
    for(uint32_t type=1;type<5;type++) {
        voice_t bass={.active=1,.sustained=1,.stage=2,.note=36,.tail=32767u<<9,.tail_amount=127};
        voice_t treble=bass;treble.note=96;
        sustain_p[0][0]=(int16_t)type;sustain_p[0][1]=3;sustain_p[0][3]=127;
        sustain_gain(&trk[0],&bass);sustain_gain(&trk[0],&treble);
        assert(type==2?bass.tail>treble.tail:bass.tail==treble.tail);
        sustain_p[0][1]=7;uint32_t gain=bass.tail;sustain_gain(&trk[0],&bass);assert(bass.tail==gain);
        sustain_p[0][3]=0;sustain_gain(&trk[0],&bass);assert(!bass.sustained&&bass.stage==3);
    }
    puts("sustain characters: clean held attacks, four distinct tails, amount/damping, piano pitch decay and zero/HOLD PASS");
}
