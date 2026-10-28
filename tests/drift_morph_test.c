/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void store(unsigned slot){ui.arm=0;snapshot_edit(slot,1);snapshot_edit(slot,1);}
static void clock_at(uint32_t units){clk_beat=units/BEAT_U;clk_pos=units%BEAT_U;}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    song.sel=0;track_t *t=&trk[0];
    assert(DRIFT_DESC[1].names==EVOLVE_DESC[1].names && DRIFT_DESC[1].max==EVOLVE_DESC[1].max);
    for(unsigned rate=0;rate<8;rate++) {
        memset(drift,0,sizeof drift);drift_p[0][0]=1;drift_p[0][1]=(int16_t)rate;
        unsigned blocks=(FS*EVOLVE_SECONDS[rate]+CTL-1)/CTL+8,wraps=0;
        for(unsigned i=0;i<blocks;i++){uint32_t old=drift[0].phase;drift_tick(t);wraps+=drift[0].phase<old;}
        assert(wraps==1);
    }
    for(unsigned rate=8;rate<NELEM(N_EVOLVE_CYCLE);rate++) {
        memset(drift,0,sizeof drift);drift_p[0][1]=(int16_t)rate;drift[0].depth=16256;drift_p[0][2]=127;
        song.playing=1;clock_at(0);drift_tick(t);int32_t seed=drift[0].rng;
        uint32_t period=(EVOLVE_UNITS[rate-8]/8u)*(BEAT_U/12u);
        clock_at(period/4);drift_tick(t);assert(drift[0].phase==0x40000000u&&drift[0].rng==seed);
        clock_at(period-1);drift_tick(t);assert(drift[0].rng==seed);
        clock_at(period);drift_tick(t);assert(!drift[0].phase&&drift[0].rng!=seed);
        clock_at(period+period/4);drift_tick(t);
        uint32_t phase=drift[0].phase;int value=drift[0].value;seed=drift[0].rng;
        song.playing=0;drift_tick(t);song.g[G_BPM]=173;drift_tick(t);
        assert(drift[0].phase==phase&&abs(drift[0].value-value)<=1&&drift[0].rng==seed);
    }
    /* A mid-cycle sync change joins without jumping and waits for the boundary. */
    drift_p[0][1]=8;song.playing=1;clock_at(0);drift_tick(t);
    clock_at(BEAT_U*16);drift_tick(t);int prior=drift[0].value;
    drift_p[0][1]=9;drift_tick(t);assert(abs(drift[0].value-prior)<=1);
    /* Real mixer: the automatic overlay must sound exactly like applying the
     * same manual position, and must restore the base parameters afterward. */
    song.playing=0;host_preset(t,0,0);t->engine=t->eng_req;t->pold=t->xf_on=0;
    t->p[P_E0]=0;t->p[P_SUS]=127;t->p[P_DIST]=t->p[P_CHOR]=t->p[P_ITYPE]=t->p[P_TFLT]=0;
    t->p[P_E4]=20;t->p[P_PAN]=-32;t->p[P_DLY]=0;store(1);
    t->p[P_E4]=100;t->p[P_PAN]=32;t->p[P_DLY]=100;store(2);
    snapshot_p[0][0]=64;snapshot_apply(t,64,0);assert(snapshot_ready(t,0));
    drift_p[0][0]=1;drift_p[0][1]=3;drift_p[0][2]=127;drift_p[0][3]=DRIFT_MORPH;
    memset(drift,0,sizeof drift);drift[0].target=DRIFT_MORPH;drift[0].from=drift[0].to=16384;drift[0].rng=1;drift[0].depth=16256;
    duck.g0=duck.g1=32767;trk_note_on(t,60,100);
    track_t base=*t;__typeof__(tflt[0]) filter=tflt[0];
    int16_t captured[2][P_COUNT];memcpy(captured,snapshot_values[0],sizeof captured);
    uint32_t midi=mo_w,random=rng_state;memset(&acc[0],0,sizeof acc[0]);
    mix_part(t,CTL,&acc[0],part_buf[0]);__typeof__(acc[0]) automatic=acc[0];
    int position=snapshot_position(0);assert(position==95);
    assert(!memcmp(base.p,t->p,sizeof base.p)&&!memcmp(captured,snapshot_values[0],sizeof captured));
    assert(mo_w==midi&&rng_state==random&&snapshot_p[0][0]==64);
    *t=base;tflt[0]=filter;memset(&acc[0],0,sizeof acc[0]);snapshot_apply(t,position,0);
    mix_part_run(t,CTL,&acc[0],part_buf[0]);assert(!memcmp(&automatic,&acc[0],sizeof automatic));
    *t=base;tflt[0]=filter;t->lk_n=1;t->lk_param[0]=P_E4;t->lk_set[0]=44;t->lk_base[0]=60;t->p[P_E4]=44;
    mix_part(t,CTL,&acc[0],part_buf[0]);assert(t->p[P_E4]==44&&t->lk_base[0]==60);t->lk_n=0;
    /* Dispatch excludes both first-ramp and fading MORPH before predicting an
     * LFO wrap, even when the caller explicitly requests that part on core 1. */
    snapshot_values[0][1][P_LRATE]=(int16_t)((t->p[P_LRATE]+1)&127);
    cpu1_units=1;drift_p[0][0]=1;drift_p[0][3]=DRIFT_MORPH;
    drift[0].target=0;drift[0].depth=0;assert(!(units_split()&1u));
    drift_p[0][3]=0;drift_p[0][0]=0;drift[0].target=DRIFT_MORPH;drift[0].depth=32;assert(!(units_split()&1u));
    /* A vowel/timbre-only morph may use the normal split, but a rate, sync or
     * currently locked timing value differing from either endpoint must not. */
    for(unsigned side=0;side<2;side++) {
        snapshot_values[0][side][P_LRATE]=t->p[P_LRATE];
        snapshot_values[0][side][P_LSYNC]=t->p[P_LSYNC];
    }
    t->lfo_ph=0;assert(snapshot_lfo_stable(0,t->p));assert(units_split()&1u);
    snapshot_values[0][1][P_LRATE]++;assert(!(units_split()&1u));snapshot_values[0][1][P_LRATE]--;
    snapshot_values[0][0][P_LSYNC]++;assert(!(units_split()&1u));snapshot_values[0][0][P_LSYNC]--;
    t->p[P_LRATE]++;assert(!(units_split()&1u));t->p[P_LRATE]--;
    t->lfo_ph=0xffffffffu;assert(!(units_split()&1u));t->lfo_ph=0;
    cpu1_units=-1;drift_p[0][0]=1;drift_p[0][3]=DRIFT_MORPH;
    /* Missing or incompatible source states are refused, never silently mixed. */
    snapshot_p[0][3]=1;assert(!snapshot_ready(t,0));snapshot_p[0][3]=3;
    static const unsigned engines[]={4,ENGI_GRAIN,ENGI_FM6,ENGI_PHYS};
    for(unsigned i=0;i<NELEM(engines);i++) {
        host_preset(t,engines[i],0);t->engine=t->eng_req;
        unsigned id=engines[i]==ENGI_FM6?P_E7:P_E0;int v=t->p[id];snapshot_p[0][3]=3;
        snapshot_values[0][0][id]=v;snapshot_values[0][1][id]=v;assert(snapshot_ready(t,0));
        snapshot_values[0][1][id]=v+1;assert(!snapshot_ready(t,0));
        snapshot_values[0][1][id]=v;t->p[id]=v+1;assert(!snapshot_ready(t,0));
    }
    host_preset(t,0,0);t->engine=t->eng_req;snapshot_p[0][3]=3;
    open_family(FAM_LFO);for(unsigned page=0;page<NPAGES;page++)if(PAGES[page].scope==SC_DRIFT)ui.page=page;
    page_entered();drift_p[0][1]=12;drift[0].phase=0x80000000u;song.playing=1;ui.msg_t=0;ui.force=1;ui_draw();ppm("drift-morph-sync");
    puts("drift/morph: shared cycles, exact beat phase, sync pause/join, real mixer/manual equivalence, unchanged bases/captures/owners/RNG, lock priority and source compatibility PASS");
    return 0;
}
