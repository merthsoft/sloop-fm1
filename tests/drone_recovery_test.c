/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void pump(uint32_t frames) {int32_t out[CTL*2];while(frames>=CTL){mix_block(out,CTL);frames-=CTL;}}
static uint32_t gates(track_t *t,uint32_t n) {
    uint32_t count=0;for(uint32_t i=0;i<NVOICE;i++)count+=t->v[i].active&&t->v[i].gate&&t->v[i].stage!=4u&&t->v[i].note==n;
    return count;
}
static voice_t *find(track_t *t,uint32_t n) {
    for(uint32_t i=0;i<NVOICE;i++)if(t->v[i].active&&t->v[i].note==n)return &t->v[i];return 0;
}
static void setup(uint32_t expanded) {
    seq_stop();memset(trk,0,sizeof trk);host_tracks_init();
    memset(drift_p,0,sizeof drift_p);memset(evolve_p,0,sizeof evolve_p);drone_was=0;
    usb.config=1;mo_r=mo_w=0;voice_expand=(uint8_t)expanded;voice_kills=shed_count=voice_recover_wait=0;
    song.playing=song.rec=transport_req=rec_wait=ft_on=0;song.sel=2;
    static const uint32_t engines[]={8,5,3};
    for(uint32_t p=0;p<NPART;p++) {
        track_t *t=&trk[p];host_preset(t,engines[p],0);t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=t->p[P_CHORD]=0;
        t->p[P_ATK]=t->p[P_DEC]=t->p[P_REL]=0;t->p[P_SUS]=127;t->p[P_SLEN]=16;t->p[P_SGATE]=127;
        sustain_p[p][0]=0;song.g[G_CH1+p]=(int16_t)(p+1);
    }
    drift_p[2][0]=3;drift_p[2][2]=64;pump(CTL*4);
}
static void pattern(void) {
    for(uint32_t p=0;p<2;p++)for(uint32_t s=0;s<16;s++) {
        step_t *st=&trk[p].step[s];st->time=ST_TIE;
        if(s==0||s==8) {
            st->time=ST_NOTE;st->n=(uint8_t)(p?4:3);st->vel=100;
            for(uint32_t j=0;j<st->n;j++)st->note[j]=(uint8_t)(48+p*12+j*3+(s?2:0));
        }
    }
    for(uint32_t s=0;s<16;s+=4)dstep_set(&TDRUM->dstep[s],0,LV_NORM,0);
}
int main(int argc,char **argv) {
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    for(uint32_t expanded=0;expanded<5;expanded++)for(uint32_t vinyl=0;vinyl<2;vinyl++) {
        setup(expanded);pattern();song.g[G_LTYPE]=(int16_t)vinyl;
        song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=64;
        transport_req=1;seq_start();pump(CTL*8);assert(voices_busy()==7);
        drone_toggle(&trk[2],72,27);drone_toggle(&trk[2],76,27);drone_toggle(&trk[2],79,27);
        uint32_t missing_seq=0,missing_drone=0;
        for(uint32_t b=0;b<FS*8/CTL;b++) {
            pump(CTL);assert(voice_load()<=VOICE_UNITS);
            for(uint32_t p=0;p<2;p++)for(uint32_t i=0;i<trk[p].seq_n;i++)missing_seq+=!gates(&trk[p],trk[p].seq_notes[i]);
            missing_drone+=!gates(&trk[2],72)+!gates(&trk[2],76)+!gates(&trk[2],79);
        }
        assert(!shed_count);
        if(expanded) {
            assert(!voice_kills&&!missing_seq&&!missing_drone);
            mi_set(MI_BUDGET,-1);assert(voice_load()==27&&!voice_kills);
            mi_set(MI_BUDGET,1); /* Lowering admission never panics existing held notes. */
        }
        else assert(voice_kills&&missing_seq&&missing_drone);
    }
    /* Every budget's exact heavy/cheap boundary, distributed over physical slots. */
    for(uint32_t level=0;level<5;level++)for(uint32_t cheap=0;cheap<2;cheap++) {
        setup(level);drift_p[2][0]=0;pump(CTL);
        uint32_t limit=VOICE_UNITS/(cheap?2u:3u);
        for(uint32_t p=0;p<NPART;p++) {
            host_preset(&trk[p],cheap?3:0,0);trk[p].p[P_VOICE]=V_POLY;trk[p].p[P_AMODE]=0;
        }
        for(uint32_t n=0;n<limit;n++)trk_note_on(&trk[n%NPART],48+n/NPART,100);
        pump(CTL*4);assert(voices_busy()==limit&&voice_load()==VOICE_UNITS&&!voice_kills);
        trk_note_on(&trk[0],80,100);pump(CTL*4);assert(voice_kills&&voice_load()==VOICE_UNITS);
    }
    /* A stolen latched note resumes locally after a quiet quarter-second. */
    setup(0);track_t *t=&trk[2];drone_toggle(t,72,27);pump(CTL*4);
    voice_t *v=find(t,72);assert(v);uint32_t age=v->age,packets=mo_w;
    voice_kill(t,v);pump(FS/8);assert(!gates(t,72)&&kh_has(t,72));
    pump(FS/4);assert(gates(t,72)==1&&v->age>age&&mo_w==packets&&voice_kills==1);
    /* Full admission must not trigger recovery steals. */
    setup(0);t=&trk[2];
    for(uint32_t p=0;p<2;p++)for(uint32_t i=0;i<(p?4u:3u);i++)trk_note_on(&trk[p],48+i*3,100);
    drone_toggle(t,72,27);pump(CTL*4);v=find(t,72);assert(v);voice_kill(t,v);pump(CTL*4);
    trk_note_on(t,80,100);pump(FS);assert(!gates(t,72)&&gates(t,80)&&voice_kills==1);
    trk_note_off(t,80);pump(FS/2);assert(gates(t,72)&&voice_kills==1);
    /* A CPU shed uses the same cooldown; held bass stays and no MIDI retrigger leaks. */
    setup(0);t=&trk[2];drone_toggle(t,60,27);drone_toggle(t,64,27);drone_toggle(t,67,27);pump(CTL*4);
    packets=mo_w;shed_voice();assert(shed_count==1);pump(FS/8);assert(gates(t,60)&&!gates(t,64));
    pump(FS/4);assert(gates(t,64)&&mo_w==packets&&shed_count==1);
    /* A deliberate reattack supersedes the stolen generation, including its fade. */
    setup(0);t=&trk[2];drone_toggle(t,72,27);pump(CTL*4);v=find(t,72);assert(v);voice_kill(t,v);
    drone_toggle(t,72,27);drone_toggle(t,72,27);pump(FS/2);
    assert(gates(t,72)==1);
    assert(!drone_lost[2][72]);
    /* Same-track reuse overwrites the old slot: remember its pitch independently. */
    setup(0);t=&trk[2];drone_toggle(t,60,27);drone_toggle(t,64,27);
    for(uint32_t p=0;p<2;p++)for(uint32_t i=0;i<3;i++)trk_note_on(&trk[p],48+i*3,100);
    drone_toggle(t,67,27);drone_toggle(t,69,27);
    assert(!gates(t,64)&&gates(t,69)&&drone_lost[2][64]);
    drone_toggle(t,69,27);pump(FS/2);
    assert(gates(t,64)&&!gates(t,69)&&!drone_lost[2][64]);
    /* Removing a drone or stopping during the cooldown cannot resurrect it. */
    for(uint32_t stop=0;stop<2;stop++) {
        setup(0);t=&trk[2];drone_toggle(t,72,27);pump(CTL*4);voice_kill(t,find(t,72));
        if(stop)seq_stop();else drone_toggle(t,72,27);
        pump(FS/2);assert(!kh_has(t,72)&&!gates(t,72));
    }
    /* Natural completion isn't a steal and must never become a repeat oscillator. */
    setup(0);t=&trk[2];host_preset(t,4,0);t->p[P_VOICE]=V_POLY;t->p[P_AMODE]=0;t->p[P_E3]=0;
    drone_toggle(t,72,27);v=find(t,72);assert(v);age=v->age;
    pump(FS*8);assert(!v->active&&!v->kill&&v->age==age&&kh_has(t,72));
    /* Actual menu writes change only runtime admission, not stored parameters. */
    setup(0);int16_t globals[G_COUNT];memcpy(globals,song.g,sizeof globals);
    mi_set(MI_BUDGET,-1);assert(!voice_expand);
    for(uint32_t level=1;level<5;level++) {mi_set(MI_BUDGET,1);assert(voice_expand==level&&VOICE_UNITS==24+6*level);}
    mi_set(MI_BUDGET,1);assert(voice_expand==4);
    mi_set(MI_BUDGET,0);assert(!voice_expand);
    for(uint32_t level=1;level<5;level++) {mi_set(MI_BUDGET,0);assert(voice_expand==level);}
    assert(!memcmp(globals,song.g,sizeof globals));
    ui.menu=1;ui.menu_sel=MI_BUDGET;ui.force=1;ui_draw();ppm("voice-budget");
    if(getenv("CORES")&&atoi(getenv("CORES"))==2)assert(cpu1_host_jobs()>0);
    puts("drone recovery: GRAIN/VOICE/LOFI scene, Vinyl parity, admission, cooldown, ownership, natural ends and native menu PASS");
}
