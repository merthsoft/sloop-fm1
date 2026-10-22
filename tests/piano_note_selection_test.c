/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
#include "../firmware/src/ui_note_select.c"
static step_t before[NSTEP];
static int same(piano_selection_t a, piano_selection_t b) { return a.step==b.step && a.pitch==b.pitch; }
static void randomized_walks(track_t *t)
{
    int32_t rng=0x5049414e;
    for(unsigned round=0;round<300;round++) {
        steps_clear(t);t->p[P_SLEN]=1+noise32(&rng)%64;
        piano_selection_t list[256];unsigned count=0;
        for(unsigned at=0;at<trk_len(t);at++) {
            step_t *st=&t->step[at];unsigned kind=noise32(&rng)%3;
            if(kind==0) {st->time=ST_REST;list[count++]=(piano_selection_t){at,0};continue;}
            if(kind==1 && at) {
                step_t prev;note_effective(t,at-1,&prev);
                if(prev.n) {st->time=ST_TIE;continue;}
            }
            st->time=ST_NOTE;st->n=1+noise32(&rng)%4;
            for(unsigned k=0;k<st->n;k++) {
                /* Separate octaves keep generated pitches unique; carried pitches
                 * are marked only when actually present in the preceding cell. */
                st->note[k]=24+k*24+noise32(&rng)%12;
                step_t prev;if(at)note_effective(t,at-1,&prev);else memset(&prev,0,sizeof prev);
                if((noise32(&rng)&1) && note_slot(&prev,st->note[k])<prev.n)st->flags|=SF_TIE(k);
                else list[count++]=(piano_selection_t){at,st->note[k]};
            }
        }
        memcpy(before,t->step,sizeof before);assert(count);
        piano_selection_t s=list[0];
        for(unsigned k=0;k<count;k++) {
            assert(same(s,list[k]));
            if(k)assert(same(piano_selection_walk(t,s,-1),list[k-1]));
            if(s.pitch)assert(piano_selection_slot(t,s)>=0);
            s=piano_selection_walk(t,s,1);
        }
        assert(same(s,list[count-1]));
        assert(!memcmp(before,t->step,sizeof before));
    }
}
int main(void)
{
    host_tracks_init();track_t *t=&trk[0];steps_clear(t);t->p[P_SLEN]=8;t->p[P_VOICE]=V_POLY;
    t->step[0]=(step_t){.time=ST_NOTE,.n=3,.note={60,64,67},.vel=90};
    t->step[2]=(step_t){.time=ST_NOTE,.n=1,.note={72},.vel=70};
    assert(!note_extend(t,0,5,1,1)); /* C held across another attack. */
    t->step[6]=(step_t){.time=ST_NOTE,.n=1,.note={60},.vel=80}; /* deliberate reattack */
    memcpy(before,t->step,sizeof before);
    piano_selection_t expected[]={{0,60},{0,64},{0,67},{2,72},{6,60},{7,0}};
    piano_selection_t s=piano_selection_at(t,0);
    for(unsigned i=0;i<NELEM(expected);i++) {
        assert(same(s,expected[i]));
        if(i)assert(same(piano_selection_walk(t,s,-1),expected[i-1]));
        s=piano_selection_walk(t,s,1);
    }
    assert(same(s,expected[NELEM(expected)-1]));
    assert(same(piano_selection_walk(t,s,INT32_MIN),expected[0]));
    assert(same(piano_selection_walk(t,s,INT32_MAX),expected[NELEM(expected)-1]));
    assert(same(piano_selection_at(t,4),expected[0]));
    assert(!memcmp(before,t->step,sizeof before));
    /* Slot compaction retains pitch identity; deleting the selected attack
     * invalidates it rather than editing whichever note slides into its slot. */
    s=(piano_selection_t){0,67};assert(piano_selection_slot(t,s)==2);
    note_remove_slot(&t->step[0],0);assert(piano_selection_slot(t,s)==1);
    note_remove_slot(&t->step[0],1);assert(piano_selection_slot(t,s)==-1);
    assert(piano_selection_slot(t,(piano_selection_t){2,60})==-1);
    assert(piano_selection_slot(t,(piano_selection_t){64,60})==-1);
    /* Legacy whole-step ties have no extra navigation stops, and stale REST
     * payload cannot create a phantom attack. */
    steps_clear(t);t->p[P_SLEN]=4;
    t->step[0]=(step_t){.time=ST_NOTE,.n=2,.note={1,127}};
    t->step[1].time=t->step[2].time=ST_TIE;
    t->step[3]=(step_t){.time=ST_REST,.n=2,.note={2,126}};
    s=piano_selection_at(t,2);assert(same(s,(piano_selection_t){0,1}));
    s=piano_selection_walk(t,s,1);assert(same(s,(piano_selection_t){0,127}));
    s=piano_selection_walk(t,s,1);assert(same(s,(piano_selection_t){3,0}));
    for(int len=1;len<=64;len++) {
        steps_clear(t);t->p[P_SLEN]=len;s=piano_selection_at(t,63);
        assert(s.step==len-1 && !s.pitch);
        assert(same(piano_selection_walk(t,s,-256),(piano_selection_t){0,0}));
    }
    randomized_walks(t);
    puts("piano selection: attack identity, mixed/legacy continuations, reverse walk, compaction, bounded empty patterns and 300 randomized patterns PASS");
    return 0;
}
