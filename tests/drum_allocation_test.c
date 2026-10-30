/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static void full(void)
{
    memset(&drums,0,sizeof drums);
    static const uint8_t notes[NDRUM]={36,38,39,43,48,51};
    for(uint32_t i=0;i<NDRUM;i++) {
        drums.v[i].active=1;drums.v[i].note=notes[i];drums.v[i].age=i+1;
    }
}
static void real_path(uint32_t kit)
{
    memset(&drums,0,sizeof drums);drum_factory_set=-2;TDRUM->p[P_E0]=(int16_t)kit;
    static const uint8_t notes[NDRUM]={36,38,39,43,48,51};
    for(uint32_t i=0;i<NDRUM;i++)drum_on(notes[i],100);
    for(uint32_t i=0;i<NDRUM;i++)assert(drums.v[i].active && drums.v[i].note==notes[i]);
    voice_t kick=drums.v[0];drums.v[1].s[7]=1234;
    drum_on(38,110); /* Full pool: restart same snare, retain kick and declick. */
    assert(!memcmp(&kick,&drums.v[0],sizeof kick));
    assert(drums.v[1].note==38 && drums.v[1].vel==110 && !drums.v[1].s[7] && drums.tail==1234);
    drums.v[2].s[7]=-345; /* Clap is now the oldest non-kick. */
    drum_on(56,100);
    assert(drums.v[2].note==56 && drums.tail==889);
    assert(!memcmp(&kick,&drums.v[0],sizeof kick));

    memset(&drums,0,sizeof drums);
    drum_on(46,100);drums.v[0].s[7]=777;drum_on(42,100);
    for(uint32_t i=0;i<NDRUM;i++)assert(!drums.v[i].active || drums.v[i].note!=46);
    assert(drums.v[0].active && drums.v[0].note==42 && drums.tail==777);
    /* A free slot still permits overlapping repeated hits, as upstream. */
    drum_on(42,100);assert(drums.v[1].active && drums.v[1].note==42);
}
int main(void)
{
    host_tracks_init();
    full();assert(drum_pick(38)==&drums.v[1]);
    full();assert(drum_pick(56)==&drums.v[1]);
    drums.v[1].note=35;assert(drum_pick(56)==&drums.v[2]);
    drums.v[4].active=0;assert(drum_pick(38)==&drums.v[4]);
    full();for(uint32_t i=0;i<NDRUM;i++)drums.v[i].note=i&1?35:36;
    assert(drum_pick(56)==&drums.v[0]); /* Saturated all-kick pool still admits hits. */
    real_path(0);real_path(DRUM_SAMPLED);
    /* Editable SYN kits use the same allocator, without altering stored sounds. */
    dsnd_t sounds[DS_LANES];ds_factory_sounds(0,sounds);full();drums.v[1].s[7]=321;
    drum_synth_on(38,100,DRUM_SYN,sounds,0,0);
    assert(drums.v[0].note==36 && drums.v[1].note==38 && drums.tail==321);
    puts("Drum allocator: free/same/oldest non-kick/all-kick priority, sampled/synth/SYN paths, six voices, choke and declick PASS");
}
