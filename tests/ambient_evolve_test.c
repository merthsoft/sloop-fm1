/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint32_t held(track_t *t) {
    uint32_t count=0;for(uint32_t n=0;n<128;n++)count+=(uint32_t)kh_has(t,n);return count;
}
static void setup(void) {
    host_tracks_init();song.sel=0;song.playing=song.rec=transport_req=rec_wait=ft_on=0;
    for(uint32_t i=0;i<NPART;i++) {
        kh_release(&trk[i]);drift_p[i][0]=2;
        trk[i].p[P_AMODE]=trk[i].p[P_CHORD]=trk[i].p[P_AHOLD]=0;
        trk[i].p[P_ROOT]=0;trk[i].p[P_SCALE]=1;trk[i].p[P_VOICE]=V_POLY;trk[i].p[P_SUS]=127;
        evolve_p[i][0]=1;evolve_p[i][1]=2;evolve_p[i][2]=127;evolve_p[i][3]=1;
        evolve_p[i][4]=-1;evolve_p[i][5]=9;
    }
    memset(evolve,0,sizeof evolve);go_home();
}
static int balance[16][128];
static void drain(void) {
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q],s=(p>>8)&255u,n=(p>>16)&127u,v=p>>24;
        if((s&240u)==144u && v)balance[s&15u][n]++;
        if((s&240u)==128u || ((s&240u)==144u && !v))balance[s&15u][n]--;
        assert(balance[s&15u][n]>=0 && balance[s&15u][n]<=1);
    }
}
int main(int argc,char **argv) {
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    setup();track_t *t=&trk[0];song.g[G_CH1]=1;song.g[G_CH2]=2;
    frame(); /* Apply drone takeover, preset and MIDI-route cleanup before adding owners. */
    drone_toggle(t,60,27);drone_toggle(t,64,27);drone_toggle(t,67,27);drain();
    step_t original[NSTEP];memcpy(original,t->step,sizeof original);
    int16_t patch[P_COUNT];memcpy(patch,t->p,sizeof patch);
    uint32_t global_rng=rng_state,changes=0;
    for(uint32_t trial=0;trial<300;trial++) {
        uint32_t before[4];memcpy(before,kh_set[0],sizeof before);
        voice_t voices[NVOICE];memcpy(voices,t->v,sizeof voices);
        evolve[0].left=1;evolve_tick(1);drain();assert(held(t)==3);
        uint32_t diff=0;
        for(uint32_t n=0;n<128;n++) {
            if(kh_has(t,n))assert(scale_mask(t)&(1u<<((n+120-t->p[P_ROOT])%12)));
            diff+=((before[n>>5]>>(n&31))&1u)!=(uint32_t)kh_has(t,n);
        }
        assert(diff==0 || diff==2);changes+=diff!=0;
        for(uint32_t v=0;v<NVOICE;v++)if(voices[v].active && voices[v].gate && kh_has(t,voices[v].note))
            assert(!memcmp(&voices[v],&t->v[v],sizeof voices[v]));
    }
    assert(changes>100 && rng_state==global_rng && !memcmp(patch,t->p,sizeof patch));
    assert(!memcmp(original,t->step,sizeof original));
    /* Absolute octave limits constrain long walks, including MIDI-end octaves. */
    kh_release(t);drain();
    static const int8_t bounds[][2]={{-1,-1},{0,0},{3,3},{3,5},{8,9},{9,9}};
    for(uint32_t b=0;b<NELEM(bounds);b++) {
        evolve_p[0][4]=bounds[b][0];evolve_p[0][5]=bounds[b][1];evolve_p[0][3]=7;
        uint32_t low=(uint32_t)(bounds[b][0]+1)*12,high=(uint32_t)(bounds[b][1]+2)*12;
        drone_toggle(t,low,27);drain();
        for(uint32_t cycle=0;cycle<500;cycle++) {
            evolve[0].left=1;evolve_tick(1);drain();assert(held(t)==1);
            for(uint32_t n=0;n<128;n++)if(kh_has(t,n))assert(n>=low&&n<high);
        }
        kh_release(t);drain();
    }
    evolve_p[0][4]=-1;evolve_p[0][5]=9;evolve_p[0][3]=1;
    drone_toggle(t,60,27);drone_toggle(t,64,27);drone_toggle(t,67,27);drain();
    /* Another track's drone/RNG is independent, including selected-track changes. */
    drone_toggle(&trk[1],48,27);drain();evolve_p[1][0]=0;
    uint32_t second[4];memcpy(second,kh_set[1],sizeof second);
    int32_t rng1=evolve[1].rng;track_select(1);
    evolve[0].left=1;evolve_tick(1);drain();
    assert(!memcmp(second,kh_set[1],sizeof second) && evolve[1].rng==rng1);
    track_select(0);kh_release(&trk[1]);drain();
    /* OFF, zero chance, inactive drone and actual timing cannot mutate pitches. */
    uint32_t before[4];memcpy(before,kh_set[0],sizeof before);
    evolve_p[0][0]=0;evolve_tick(FS*10u);assert(!memcmp(before,kh_set[0],sizeof before));
    evolve_p[0][0]=1;evolve_p[0][2]=0;evolve[0].left=1;evolve_tick(1);
    assert(!memcmp(before,kh_set[0],sizeof before));
    evolve_p[0][2]=127;evolve[0].left=FS*5u;evolve_tick(FS*5u-1);assert(evolve[0].left==1);
    assert(!memcmp(before,kh_set[0],sizeof before));
    evolve_p[0][1]=7;evolve_tick(1);assert(evolve[0].left==FS*160u);
    kh_release(t);drain();assert(!evolve[0].left);
    /* One note shared with the sequencer is never stolen by evolution. */
    drone_toggle(t,60,27);drain();t->seq_n=1;t->seq_notes[0]=60;
    evolve[0].rate=7;evolve[0].left=1;evolve_tick(1);assert(kh_has(t,60));
    t->seq_n=0;
    midi_scale_input(t,60,60,100,1);evolve[0].left=1;evolve_tick(1);assert(kh_has(t,60));
    midi_scale_input(t,60,60,0,0);
    kb_kind[0]=KS_NOTE;kb_n[0]=1;kb_nt[0][0]=60;kb_trk[0]=0;
    evolve[0].left=1;evolve_tick(1);assert(kh_has(t,60));
    kb_kind[0]=KS_NONE;kb_n[0]=0;
    kh_release(t);drain();
    /* All scales, roots, range extremes and MIDI boundaries preserve bounds. */
    for(uint32_t scale=0;scale<NSCALES;scale++)for(uint32_t root=0;root<12;root++) {
        t->p[P_SCALE]=(int16_t)scale;t->p[P_ROOT]=(int16_t)root;
        drone_toggle(t,root,27);evolve_p[0][3]=7;
        for(uint32_t i=0;i<12;i++){evolve[0].left=1;evolve_tick(1);drain();assert(held(t)==1);}
        kh_release(t);drain();drone_toggle(t,127,27);evolve[0].left=1;evolve_tick(1);drain();
        assert(held(t)==1);kh_release(t);drain();
    }
    for(uint32_t c=0;c<16;c++)for(uint32_t n=0;n<128;n++)assert(!balance[c][n]);
    /* Real recorder stores evolved output, not a recipe or patch mutation. */
    t->p[P_ROOT]=0;t->p[P_SCALE]=1;t->p[P_SLEN]=64;t->p[P_SDIV]=2;
    steps_clear(t);evolve_p[0][1]=2;evolve_p[0][3]=1;
    seq_start();song.rec=1;drone_toggle(t,60,27);drain();
    assert(t->rh_n==1);
    evolve[0].rate=2;evolve[0].left=1;evolve_tick(1);drain();
    uint32_t moved=0;for(uint32_t n=0;n<128;n++)if(kh_has(t,n))moved=n;
    assert(moved!=60 && t->rh_n==1 && t->rh_note[0]==moved);
    uint32_t recorded=0;
    for(uint32_t s=0;s<64;s++)for(uint32_t j=0;j<t->step[s].n;j++)recorded+=t->step[s].note[j]==moved;
    assert(recorded);song.rec=0;seq_stop();drain();
    /* Exercise the real audio clock across a full cycle while recording holds. */
    steps_clear(t);seq_start();song.rec=1;drone_toggle(t,60,27);drain();
    evolve[0].left=FS*5u;int32_t output[CTL*2];
    for(uint32_t frame=0;frame<FS*6u/CTL;frame++){mix_block(output,CTL);drain();}
    assert(held(t)==1 && !kh_has(t,60) && t->rh_n==1);
    song.rec=0;seq_stop();drain();
    for(uint32_t c=0;c<16;c++)for(uint32_t n=0;n<128;n++)assert(!balance[c][n]);
    /* All synced divisions use the transport phase, not an unrelated countdown. */
    for(uint32_t rate=8;rate<NELEM(N_EVOLVE_CYCLE);rate++) {
        evolve_p[0][1]=(int16_t)rate;song.playing=1;clk_beat=clk_pos=0;
        drone_toggle(t,60,27);evolve[0].running=0;evolve_tick(CTL);assert(kh_has(t,60));
        uint32_t units=EVOLVE_UNITS[rate-8u],rng=(uint32_t)evolve[0].rng;
        clk_beat=(units-1u)/96u;clk_pos=((units-1u)%96u)*BEAT_U/96u;
        evolve_tick(CTL);assert(kh_has(t,60)&&(uint32_t)evolve[0].rng==rng&&evolve_phase(0)>110);
        clk_beat=units/96u;clk_pos=(units%96u)*BEAT_U/96u;
        evolve_tick(CTL);drain();assert(held(t)==1&&!kh_has(t,60));
        uint32_t snapshot[4];memcpy(snapshot,kh_set[0],sizeof snapshot);rng=(uint32_t)evolve[0].rng;
        evolve_tick(CTL);song.g[G_BPM]=123;evolve_tick(CTL);
        assert(!memcmp(snapshot,kh_set[0],sizeof snapshot)&&(uint32_t)evolve[0].rng==rng);
        song.playing=0;clk_beat=units*2u/96u;clk_pos=(units*2u%96u)*BEAT_U/96u;evolve_tick(CTL);
        assert(!evolve_phase(0)&&!memcmp(snapshot,kh_set[0],sizeof snapshot));
        song.playing=1;evolve_tick(CTL);assert(!memcmp(snapshot,kh_set[0],sizeof snapshot));
        kh_release(t);drain();song.playing=0;
    }
    /* The page owns runtime knobs, not upstream patch IDs or parameter locks. */
    open_family(FAM_LFO);for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_EVOLVE && PAGES[p].id[0]==0)ui.page=p;
    page_entered();frame();uint8_t lock=ui.lock_par;memcpy(patch,t->p,sizeof patch);
    encs[panel.enc[EN_K1]]=1;frame();assert(evolve_p[0][0]==1);
    encs[panel.enc[EN_K2]]=-100;frame();assert(!evolve_p[0][1]);
    encs[panel.enc[EN_K3]]=100;frame();assert(evolve_p[0][2]==127);
    encs[panel.enc[EN_K4]]=-100;frame();assert(evolve_p[0][3]==1);
    assert(ui.lock_par==lock && !memcmp(patch,t->p,sizeof patch));
    assert(!strcmp(ui.big_l[2],"CHANCE") && !strcmp(ui.big_l[3],"RANGE"));
    /* The displayed cycle follows scheduler state and updates without knob input. */
    evolve_p[0][1]=2;drone_toggle(t,60,27);evolve[0].left=FS*5u;
    ui.msg_t=0;ui.hot_t=0;ui.force=1;ui_draw();uint16_t shown[240*240];memcpy(shown,screen,sizeof shown);
    evolve[0].left=FS*5u/2u;draw_graph();assert(memcmp(shown,screen,sizeof shown));
    memcpy(shown,screen,sizeof shown);ui.force=1;draw_graph();assert(!memcmp(shown,screen,sizeof shown));
    ppm("ambient-evolve");
    evolve_p[0][1]=14;clk_beat=clk_pos=0;song.playing=0;ui.force=1;ui_draw();memcpy(shown,screen,sizeof shown);
    song.playing=1;ui.force=0;draw_graph();assert(memcmp(shown,screen,sizeof shown));
    ppm("ambient-evolve-sync");song.playing=0;kh_release(t);drain();
    /* Both octave controls are native runtime values; crossing moves the other
     * endpoint to keep a valid range, without retuning existing held notes. */
    drone_toggle(t,60,27);drain();evolve_p[0][1]=2;evolve[0].rate=2;evolve[0].left=FS*5u;
    for(uint32_t page=0;page<NPAGES;page++)if(PAGES[page].scope==SC_EVOLVE && PAGES[page].id[0]==4)ui.page=page;
    page_entered();frame();uint32_t owners[4];memcpy(owners,kh_set[0],sizeof owners);
    encs[panel.enc[EN_K1]]=100;frame();assert(evolve_p[0][4]==9&&evolve_p[0][5]==9);
    encs[panel.enc[EN_K2]]=-100;frame();assert(evolve_p[0][4]==-1&&evolve_p[0][5]==-1);
    assert(!memcmp(owners,kh_set[0],sizeof owners)&&kh_has(t,60));
    evolve[0].left=1;evolve_tick(1);drain();assert(kh_has(t,60)); /* outside bounds: no forced octave jump */
    int16_t *empty;assert(!page_desc(cur_page(),2,&empty)&&!empty);
    assert(!page_desc(cur_page(),3,&empty)&&!empty);
    evolve_p[0][4]=3;evolve_p[0][5]=5;ui.force=1;ui_draw();ppm("ambient-evolve-bounds");
    kh_release(t);drain();
    track_select(TRK_DRUM);int16_t *v;for(unsigned k=0;k<4;k++)assert(!page_desc(cur_page(),k,&v));
    puts("evolution: scales/roots/bounds, one-note changes, timing/chance/OFF, ownership, MIDI balance, patch preservation and native knobs PASS");
}
