/* SPDX-License-Identifier: GPL-3.0-only */
/* Current-note RNG eligibility and bounded measured whole-unit partitioning.
 * Synthetic ticks test policy, not target performance or deadline compliance. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static void setup(void)
{
    host_tracks_init();punch_clear();voice_expand=4;cpu1_units=-1;
    memset(unit_time,0,sizeof unit_time);unit_last_mask=0;
    for(uint32_t k=0;k<NPART;k++) {
        memset(trk[k].v,0,sizeof trk[k].v);
        host_preset(&trk[k],0,0);trk[k].p[P_VOICE]=V_POLY;
        trk[k].p[P_LRATE]=trk[k].p[P_LSYNC]=0;trk[k].lfo_ph=0;
        trk[k].xf=trk[k].xf_on=trk[k].pold=0;
        drift[k].depth=drift_p[k][0]=0;
    }
}
static void profile(uint32_t k,uint32_t ticks)
{
    unit_time[k].ticks=ticks;
    unit_time[k].identity=k<NPART?unit_identity(&trk[k]):0;
    unit_time[k].voices=k<NPART?mix_active(&trk[k]):1;
}
static void note(uint32_t k,uint32_t pitch)
{trk_note_on(&trk[k],pitch,100);}

static void formant_activity(void)
{
    setup();host_preset(&trk[0],5,0);host_preset(&trk[1],5,0);
    note(0,60);cpu1_units=U_ALL;
    assert(units_split()&1u); /* no previous rendered count required */
    note(1,64);assert(!(units_split()&3u)); /* same-block note-on */
    trk_note_off(&trk[1],64);assert(!(units_split()&3u)); /* release tail */
    trk[1].xf_on=1;trk[1].xf=XF_BLOCKS;trk[1].eng_req=0;
    assert(!(units_split()&3u)); /* fading old FORMANT still uses its RNG */
    trk[1].xf=0;engine_block(&trk[1]);
    assert(trk[1].engine==0 && !mix_active(&trk[1]));
    assert(units_split()&1u);
    trk[1].xf_on=1;trk[1].xf=0;trk[1].eng_req=5;
    trk[1].xp_n=1;trk[1].xp_note[0]=67;trk[1].xp_vel[0]=100;
    engine_block(&trk[1]);assert(trk[1].engine==5 && mix_active(&trk[1]));
    assert(!(units_split()&3u)); /* new engine adopted pending note */
    memset(trk[1].v,0,sizeof trk[1].v);
    assert(units_split()&1u);
}

static void partitions(void)
{
    setup();for(uint32_t k=0;k<NPART;k++)note(k,60+4*k);
    profile(0,6000);profile(1,9000);profile(2,11000);profile(NPART,2000);
    assert(units_split()==3u); /* two synths improve the old one-synth limit */
    unit_last_mask=12;assert(units_split()==12u); /* equal peak keeps stable split */
    unit_time[2].ticks=14000;assert(units_split()==12u);
    unit_time[2].ticks=30000;
    assert(units_split()==4u); /* clear gain overrides previous preference */
    trk[2].lfo_ph=UINT32_MAX;
    assert(!(units_split()&4u)); /* clock wrap exclusion applies to all subsets */
    trk[2].lfo_ph=0;
    cpu1_units=5;assert(units_split()==5u); /* manual masks retain semantics */
    for(uint32_t k=0;k<NTRK;k++)assert(!unit_time[k].ticks);
    cpu1_units=-1;
    profile(0,6000);profile(1,9000);profile(2,11000);profile(NPART,2000);
    trk[0].preset++;
    assert(units_split()==(4u|U_DRUMS)); /* invalid identity uses whole fallback */
    profile(0,6000);unit_time[0].voices=0;
    assert(units_split()==(4u|U_DRUMS)); /* empty -> live invalidates profile */
    profile(0,6000);unit_time[0].identity^=1u<<16;
    assert(units_split()==(4u|U_DRUMS)); /* model identity */
    profile(0,6000);trk[0].xf_on=1;trk[0].xf=XF_BLOCKS;
    assert(units_split()==(4u|U_DRUMS)); /* fade identity */
    cpu1_units=0;assert(!units_split());
    for(uint32_t k=0;k<NTRK;k++)assert(!unit_time[k].ticks);
    unit_time_set(0,0,1,7);assert(!unit_time[0].ticks);
    unit_time_set(0,UINT32_MAX-9u,1,7);assert(unit_time[0].ticks==10);
    unit_time_set(0,0u-300000u,1,7);assert(!unit_time[0].ticks);
}

int main(int argc,char **argv)
{
    if(argc>1) {
        setenv("CPU1_TIMEOUT_AT","1",1);setup();note(0,60);
        for(uint32_t k=0;k<NTRK;k++)profile(k,1000+100*k);
        int32_t out[CTL*2];mix_block(out,CTL);
        assert(cpu1_host_timeouts()==1 && !cpu1_on());
        for(uint32_t k=0;k<NTRK;k++)assert(!unit_time[k].ticks);
        assert(!unit_last_mask);
        puts("Unit scheduler timeout invalidation PASS");return 0;
    }
    formant_activity();partitions();
    puts("Unit scheduler current FORMANT activity, measured subsets, RNG guards, identity/fresh-note/manual/timer invalidation PASS");
    return 0;
}
