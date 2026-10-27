/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static void store(uint32_t slot)
{
    ui.arm=0;snapshot_edit(slot,1);assert(ui.arm);
    snapshot_edit(slot,1);assert(!ui.arm);
}

int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    knob_accel_off=1;
    for(uint32_t engine=0;engine<NENGINES;engine++) {
        song.sel=0;track_t *t=&trk[0];host_preset(t,engine,0);
        memset(snapshot_p,0,sizeof snapshot_p);
        int16_t lo[P_COUNT],hi[P_COUNT];
        for(uint32_t id=0;id<P_COUNT;id++) {
            const param_desc_t *d=id>=P_E0?track_desc(t,id):&TP[id];
            lo[id]=t->p[id];hi[id]=t->p[id];
            if(snapshot_param(id)){lo[id]=d->min;hi[id]=d->max;}
        }
        memcpy(t->p,lo,sizeof lo);store(1);assert(snapshot_p[0][3]==1);
        snapshot_edit(0,64);assert(!memcmp(t->p,lo,sizeof lo)); /* missing B */
        memcpy(t->p,hi,sizeof hi);store(2);assert(snapshot_p[0][3]==3);
        step_t pattern[NSTEP];memcpy(pattern,t->step,sizeof pattern);
        voice_t voices[NVOICE];memcpy(voices,t->v,sizeof voices);
        uint32_t out=mo_w,rng=rng_state;
        for(uint32_t pos=0;pos<128;pos++) {
            snapshot_p[0][0]=0;snapshot_edit(0,(int32_t)pos);
            for(uint32_t id=0;id<P_COUNT;id++) {
                const param_desc_t *d=id>=P_E0?track_desc(t,id):&TP[id];
                int expected=snapshot_param(id)?d->names?(pos<64?lo[id]:hi[id]):lo[id]+(hi[id]-lo[id])*(int)pos/127:hi[id];
                if(t->p[id]!=expected)fprintf(stderr,"engine %u pos %u id %u got %d expected %d mask %d\n",engine,pos,id,t->p[id],expected,snapshot_p[0][3]);
                assert(t->p[id]==expected);
                assert(t->p[id]>=d->min && t->p[id]<=d->max);
            }
        }
        assert(!memcmp(pattern,t->step,sizeof pattern));
        assert(!memcmp(voices,t->v,sizeof voices));assert(mo_w==out && rng_state==rng);
        assert(!snapshot_p[1][3] && !snapshot_p[2][3]);
        /* Locked filter keeps its instantaneous value while its base morphs. */
        t->lk_n=1;t->lk_param[0]=P_TFLT;t->lk_set[0]=17;t->p[P_TFLT]=17;t->lk_base[0]=-4;
        snapshot_p[0][0]=0;snapshot_edit(0,127);
        assert(t->p[P_TFLT]==17 && t->lk_base[0]==hi[P_TFLT]);
        locks_restore(t);assert(t->p[P_TFLT]==hi[P_TFLT]);
        sound_change(t);assert(!snapshot_p[0][3]);
    }
    /* Real page navigation / knobs and per-track isolation. */
    song.sel=0;host_preset(&trk[0],0,0);frame();open_family(FAM_EDIT);
    for(uint32_t n=0;n<NPAGES && cur_page()->scope!=SC_MORPH;n++)page_walk(1);
    assert(cur_page()->scope==SC_MORPH);
    edit_param(1,1);edit_param(1,1);
    trk[0].p[P_TFLT]=55;edit_param(2,1);edit_param(2,1);
    edit_param(0,-127);assert(trk[0].p[P_TFLT]==snapshot_values[0][0][P_TFLT]);
    edit_param(0,127);assert(trk[0].p[P_TFLT]==55);
    ui.msg_t=0;ui.force=1;frame();ppm("track-morph");
    int16_t saved[4];memcpy(saved,snapshot_p[0],sizeof saved);
    song.sel=3;snapshot_edit(1,1);snapshot_edit(0,1);
    assert(!memcmp(saved,snapshot_p[0],sizeof saved));
    puts("snapshots: all-engine endpoints/ranges, discrete midpoint, locks, owners, no MIDI/RNG, track isolation, invalidation and actual page controls PASS");
    return 0;
}
