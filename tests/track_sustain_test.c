/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void pump(uint32_t frames) {int32_t out[CTL*2];while(frames>=CTL){mix_block(out,CTL);frames-=CTL;}}
static voice_t *note(track_t *t,uint32_t n) {for(uint32_t i=0;i<NVOICE;i++)if(t->v[i].active&&t->v[i].note==n)return &t->v[i];return 0;}
static void setup(uint32_t engine) {
    usb.config=1;seq_stop();host_tracks_init();song.sel=0;song.playing=song.rec=transport_req=rec_wait=ft_on=0;
    fm1_in.notes=fm1_in.buttons=0;go_home();
    for(uint32_t i=0;i<NPART;i++){drift_p[i][0]=evolve_p[i][0]=sustain_p[i][0]=0;}
    set_engine_of(TSEL,engine);apply_preset_to(TSEL,0);frame();frames(20);
    TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_AMODE]=TSEL->p[P_CHORD]=0;
    TSEL->p[P_ATK]=TSEL->p[P_DEC]=TSEL->p[P_REL]=0;TSEL->p[P_SUS]=127;
    sustain_p[0][0]=1;sustain_p[0][1]=7;sustain_p[0][2]=0;sustain_p[0][3]=127;
}
int main(int argc,char **argv) {
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    setup(0);track_t *t=TSEL;
    trk_note_on(t,60,100);pump(FS/8);voice_t *v=note(t,60);assert(v&&v->gate);
    trk_note_off(t,60);assert(!v->gate&&v->sustained&&voice_engine_held(v)&&v->stage!=3);
    uint32_t level=v->tail;pump(FS*2);assert(v->active&&v->tail==level);
    /* A fresh note is independent; turning the effect off never releases it. */
    trk_note_on(t,64,100);voice_t *held=note(t,64);assert(held&&held->gate&&!held->sustained);
    sustain_p[0][0]=0;pump(CTL);assert(!v->sustained&&v->stage==3&&held->gate);
    pump(FS/4);assert(!v->active&&held->gate);trk_note_off(t,64);pump(FS/4);
    /* Finite tails fall monotonically and terminate without ever becoming held owners. */
    sustain_p[0][0]=1;sustain_p[0][1]=0;trk_note_on(t,60,100);pump(FS/8);v=note(t,60);trk_note_off(t,60);
    uint32_t ticks=0;level=32767u<<9;
    while(v->active&&ticks++<FS/CTL){pump(CTL);assert(!v->gate&&v->tail<=level);level=v->tail;}
    assert(!v->active&&!v->sustained&&ticks>FS/(CTL*8)&&ticks<FS/CTL);
    /* Decay labels represent approximately RT60, including the long settings. */
    static const uint32_t milliseconds[]={500,1000,2000,4000,8000,16000,32000};
    for(uint32_t choice=0;choice<7;choice++) {
        sustain_p[0][1]=(int16_t)choice;
        voice_t tail={.active=1,.sustained=1,.stage=2,.tail=32767u<<9};
        uint32_t ticks=0;while(tail.active&&ticks<FS*40u/CTL){sustain_gain(t,&tail);ticks++;}
        uint32_t ms=ticks*CTL*1000u/FS;
        assert(ms>milliseconds[choice]*9u/10u&&ms<milliseconds[choice]*11u/10u);
    }
    /* Repeated pitches deliberately attack again and clear old tail state. */
    sustain_p[0][1]=7;trk_note_on(t,60,100);pump(FS/8);v=note(t,60);trk_note_off(t,60);uint32_t age=v->age;
    trk_note_on(t,60,90);assert(v->age>age&&v->gate&&!v->sustained&&!v->tail);
    trk_note_off(t,60);trk_all_off(t);assert(!v->sustained&&!v->gate);
    /* Tail voices remain eligible for release-first stealing; held bass survives. */
    setup(0);t=TSEL;trk_note_on(t,36,100);pump(CTL);
    for(uint32_t n=60;n<67;n++){trk_note_on(t,n,100);pump(CTL);trk_note_off(t,n);}
    assert(voices_busy()<=NVOICE);trk_note_on(t,80,100);assert(note(t,36)&&note(t,36)->gate&&voices_busy()<=NVOICE);
    /* Physical tap/MIDI pedal and DRONE retain their existing logical ownership. */
    setup(0);t=TSEL;sus_set(t,1);input_on(t,60,100);input_off(t,60);v=note(t,60);
    assert(v&&v->gate&&!v->sustained);sus_set(t,0);assert(!v->gate&&v->sustained);
    trk_all_off(t);drift_p[0][0]=2;frame();key(0);uint32_t n=kb_map(t,0);v=note(t,n);
    assert(v&&drone_held(t,n));trk_note_off(t,n);assert(v->gate&&!v->sustained);
    key(0);assert(!drone_held(t,n)&&!v->gate&&v->sustained);
    trk_all_off(t);assert(!v->sustained);drift_p[0][0]=0;
    /* A local acoustic tail never delays the physical key's outgoing MIDI-off. */
    setup(0);t=TSEL;song.g[G_CH1]=1;frame();mo_r=mo_w=0;uint32_t played=kb_map(t,0);key(0);v=note(t,played);
    assert(v&&v->sustained&&!v->gate);uint32_t ons=0,offs=0;
    while(mo_r!=mo_w) {
        uint32_t packet=midi_out_q[mo_r++%MOUT_Q],status=(packet>>8)&240u;
        if(((packet>>16)&127u)==played){ons+=status==144u;offs+=status==128u;}
    }
    assert(ons==1&&offs==1);pump(FS/8);assert(mo_r==mo_w&&v->active);
    /* Independent ties keep their gate; only notes that actually end become tails. */
    setup(0);t=TSEL;steps_clear(t);t->p[P_SLEN]=4;t->p[P_SDIV]=2;
    t->step[0]=(step_t){.time=ST_NOTE,.n=1,.note={60},.vel=100};
    t->step[1]=(step_t){.time=ST_NOTE,.n=2,.note={60,64},.vel=100,.flags=SF_CONT(0)};
    seq_start();pump(CTL);v=note(t,60);assert(v);uint32_t attack=v->age,wait=0;
    while(t->seq_idx!=1&&wait++<FS/CTL)pump(CTL);
    assert(wait<FS/CTL&&v->gate&&!v->sustained&&v->age==attack&&note(t,64));
    wait=0;while(t->seq_idx!=2&&wait++<FS/CTL)pump(CTL);
    assert(wait<FS/CTL&&!v->gate&&v->sustained&&note(t,64)->sustained);
    seq_stop();pump(CTL);assert(!v->sustained);
    /* Engine-specific gates stay down only for the local ringing tail. */
    for(uint32_t engine=0;engine<NENGINES;engine++) {
        setup(engine);t=TSEL;trk_note_on(t,60,100);pump(FS/16);v=note(t,60);
        if(v) {trk_note_off(t,60);assert(!v->gate&&v->sustained);pump(FS/8);
            if(v->active)assert(voice_engine_held(v));
            trk_all_off(t);assert(!v->sustained&&!voice_engine_held(v));}
    }
    setup(ENGI_FM6);t=TSEL;trk_note_on(t,60,100);pump(FS/8);v=note(t,60);assert(v);
    trk_note_off(t,60);pump(FS/8);fm6_note_t *fm=fm6_note_of(t,v);assert(fm&&fm->down);
    sustain_p[0][0]=0;pump(CTL);assert(!fm->down&&!v->sustained);
    /* Multiple ringing parts traverse the actual two-core render plan. */
    for(uint32_t type=1;type<5;type++) {
    setup(0);
    for(uint32_t part=1;part<NPART;part++){set_engine_of(&trk[part],0);apply_preset_to(&trk[part],0);}
    frames(30);
    for(uint32_t part=0;part<NPART;part++) {
        track_t *t=&trk[part];t->p[P_VOICE]=V_POLY;t->p[P_SUS]=127;
        sustain_p[part][0]=(int16_t)type;sustain_p[part][1]=7;
        sustain_p[part][2]=80;sustain_p[part][3]=64;
        trk_note_on(t,48+part*7,100);trk_note_on(t,52+part*7,100);
    }
    pump(FS/4);
    for(uint32_t part=0;part<NPART;part++) {
        trk_note_off(&trk[part],48+part*7);trk_note_off(&trk[part],52+part*7);
    }
    pump(FS/4);assert(voices_busy()<=NVOICE);
    if(getenv("CORES")&&atoi(getenv("CORES"))==2)assert(cpu1_host_jobs()>0);
    for(uint32_t part=0;part<NPART;part++)trk_all_off(&trk[part]);
    }
    /* Native controls are per track and never edit patch/MIDI-learn IDs. */
    setup(0);t=TSEL;open_family(FAM_ENV);for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_SUSTAIN)ui.page=p;
    page_entered();frame();int16_t patch[P_COUNT];memcpy(patch,t->p,sizeof patch);uint8_t lock=ui.lock_par;
    sustain_p[0][0]=0;encs[panel.enc[EN_K1]]=1;frame();assert(sustain_p[0][0]==1);
    encs[panel.enc[EN_K2]]=100;frame();assert(sustain_p[0][1]==7);
    assert(!memcmp(patch,t->p,sizeof patch)&&lock==ui.lock_par);
    int16_t *value;assert(page_desc(cur_page(),2,&value)&&value==&sustain_p[0][2]);
    assert(page_desc(cur_page(),3,&value)&&value==&sustain_p[0][3]);
    sustain_p[0][0]=2;sustain_p[0][1]=4;sustain_p[0][2]=48;
    ui.msg_t=0;ui.force=1;ui_draw();ppm("track-sustain");track_select(1);assert(!sustain_p[1][0]);
    track_select(TRK_DRUM);assert(!page_desc(cur_page(),0,&value));
    puts("sustain: decay/HOLD/OFF, released-voice priority, repeated notes, pedal/drone ownership, all engine gates and native controls PASS");
}
