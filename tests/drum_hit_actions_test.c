/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include "../firmware/src/drum_hit_actions.c"

static track_t snapshot, expected;
static void unchanged(track_t *t) { assert(!memcmp(t, &snapshot, sizeof *t)); }
static void seed(track_t *t, unsigned lane, unsigned source)
{
    memset(t, 0x95, sizeof *t);
    t->p[P_SLEN] = NSTEP;
    for (unsigned i = 0; i < NSTEP; ++i) {
        for (unsigned l = 0; l < DRUM_LANES; ++l)
            dstep_set(&t->dstep[i], l, (i+l)%4, (i+2*l)%4);
        dstep_clr(&t->dstep[i], lane);
    }
    dstep_set(&t->dstep[source], lane, source%4, (source/4)%4);
    snapshot = *t;
}
int main(void)
{
    track_t *t = &trk[TRK_DRUM];
    for (unsigned lane = 0; lane < DRUM_LANES; ++lane)
        for (unsigned source = 0; source < NSTEP; ++source)
            for (unsigned move = 0; move < 2; ++move) {
                unsigned dest = (source + 31) % NSTEP;
                seed(t, lane, source);
                assert(drum_hit_place(t,lane,source,dest,move,0)==DRUM_HIT_OK);
                unchanged(t);
                expected = *t;
                dstep_set(&expected.dstep[dest],lane,source%4,(source/4)%4);
                if (move) dstep_clr(&expected.dstep[source],lane);
                assert(drum_hit_place(t,lane,source,dest,move,1)==DRUM_HIT_OK);
                assert(!memcmp(t,&expected,sizeof *t));
                assert(dstep_has(&t->dstep[dest],lane));
                assert(dstep_lvl(&t->dstep[dest],lane)==source%4);
                assert(dstep_rat(&t->dstep[dest],lane)==(source/4)%4);
                assert(dstep_has(&t->dstep[source],lane)==!move);
                seed(t,lane,source);
                assert(drum_hit_delete(t,lane,source,0)==DRUM_HIT_OK); unchanged(t);
                expected=*t; dstep_clr(&expected.dstep[source],lane);
                assert(drum_hit_delete(t,lane,source,1)==DRUM_HIT_OK);
                assert(!memcmp(t,&expected,sizeof *t));
                assert(!dstep_has(&t->dstep[source],lane));
            }
    seed(t,15,0);
    for (unsigned write=0;write<2;++write) {
        assert(drum_hit_place(t,15,0,0,1,write)==DRUM_HIT_NOOP); unchanged(t);
        assert(drum_hit_place(t,15,0,0,0,write)==DRUM_HIT_OCCUPIED); unchanged(t);
        assert(drum_hit_place(t,15,1,2,1,write)==DRUM_HIT_EMPTY); unchanged(t);
        assert(drum_hit_delete(t,15,1,write)==DRUM_HIT_EMPTY); unchanged(t);
        const unsigned bad[]={NSTEP,INT_MAX,(unsigned)INT_MIN,UINT_MAX};
        for(unsigned i=0;i<sizeof bad/sizeof *bad;++i) {
            assert(drum_hit_place(t,15,0,bad[i],1,write)==DRUM_HIT_BOUNDS); unchanged(t);
            assert(drum_hit_place(t,15,bad[i],1,0,write)==DRUM_HIT_BOUNDS); unchanged(t);
            assert(drum_hit_delete(t,15,bad[i],write)==DRUM_HIT_BOUNDS); unchanged(t);
            assert(drum_hit_delete(t,bad[i],0,write)==DRUM_HIT_BOUNDS); unchanged(t);
        }
        assert(drum_hit_delete(t,16,0,write)==DRUM_HIT_BOUNDS); unchanged(t);
        assert(drum_hit_place(NULL,0,0,1,1,write)==DRUM_HIT_BOUNDS);
        assert(drum_hit_delete(NULL,0,0,write)==DRUM_HIT_BOUNDS);
    }
    dstep_set(&t->dstep[1],15,3,2); snapshot=*t;
    for(unsigned move=0;move<2;++move) for(unsigned write=0;write<2;++write) {
        assert(drum_hit_place(t,15,0,1,move,write)==DRUM_HIT_OCCUPIED); unchanged(t);
    }
    for(unsigned len=1;len<=NSTEP;++len) {
        seed(t,0,0); t->p[P_SLEN]=len; snapshot=*t;
        assert(drum_hit_place(t,0,0,len,1,1)==DRUM_HIT_BOUNDS); unchanged(t);
        assert(drum_hit_delete(t,0,len,1)==DRUM_HIT_BOUNDS); unchanged(t);
        assert(drum_hit_place(t,0,0,len-1,1,0)==(len==1?DRUM_HIT_NOOP:DRUM_HIT_OK)); unchanged(t);
    }
    puts("drum hit actions: all assertions passed");
    return 0;
}
