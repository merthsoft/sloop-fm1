/* SPDX-License-Identifier: GPL-3.0-only */
/* Physical controls through production ui_input, with audio running each frame. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void expect_page(const char *title)
{
    assert(!ui.home && !strcmp(cur_page()->title,title));
}
static void select_turn(int32_t amount)
{ encs[panel.enc[EN_SELECT]]=amount;frame(); }
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    song.sel=1;go_home();frame();
    tap(B_SEQ);expect_page("STEP");
    track_t *paint=&trk[1];
    /* Whole-step move keeps a tied chord and all timing/lock metadata together. */
    steps_clear(paint);paint->p[P_SLEN]=32;cursor_set(10);
    paint->step[10].time=ST_NOTE;paint->step[10].n=2;
    paint->step[10].note[0]=60;paint->step[10].note[1]=64;
    paint->step[11].time=paint->step[12].time=ST_TIE;
    paint->micro[11]=-7;step_fill_set(paint,12,FC_FILL);
    assert(lock_set(paint,10,P_LEVEL,88));
    step_t before_move[NSTEP];memcpy(before_move,paint->step,sizeof before_move);
    press(B_OCTUP);encs[panel.enc[EN_K3]]=4;frame();
    assert(ui.cursor==14&&paint->step[10].time==ST_REST&&paint->step[11].time==ST_REST);
    assert(paint->step[14].n==2&&paint->step[15].time==ST_TIE&&paint->step[16].time==ST_TIE);
    assert(paint->micro[15]==-7&&step_fill(paint,16)==FC_FILL&&step_locked(paint,14));
    encs[panel.enc[EN_K3]]=-1;frame();assert(ui.cursor==13&&paint->micro[14]==-7);
    paint->step[16].time=ST_NOTE;paint->step[16].n=1;paint->step[16].note[0]=72;
    encs[panel.enc[EN_K3]]=3;frame();assert(ui.cursor==13&&paint->step[16].note[0]==72);
    step_clear(&paint->step[16]);release(B_OCTUP);
    assert(undo_swap(0)&&!memcmp(before_move,paint->step,sizeof before_move));
    assert(paint->micro[11]==-7&&step_fill(paint,12)==FC_FILL&&step_locked(paint,10));
    assert(undo_swap(1)&&paint->micro[14]==-7&&step_locked(paint,13));
    assert(trk[0].step[10].time==ST_REST);
    /* Own ties are usable targets even when sustain reaches the pattern end. */
    steps_clear(paint);paint->p[P_SLEN]=16;cursor_set(0);
    paint->step[0]=(step_t){.note={60,64,67},.n=3,.time=ST_NOTE,.vel=87,.flags=SF_ACCENT};
    for(uint32_t i=1;i<16;i++)paint->step[i].time=ST_TIE;
    paint->micro[0]=-5;paint->micro[8]=7;step_fill_set(paint,0,FC_NOFILL);
    assert(lock_set(paint,0,P_LEVEL,88)&&lock_set(paint,2,P_LEVEL,33)&&lock_set(paint,8,P_PAN,22));
    memcpy(before_move,paint->step,sizeof before_move);
    press(B_OCTUP);encs[panel.enc[EN_K3]]=2;frame();release(B_OCTUP);
    assert(ui.cursor==2&&paint->step[0].time==ST_REST&&paint->step[1].time==ST_REST);
    assert(paint->step[2].n==3&&paint->step[2].vel==87&&paint->micro[2]==-5);
    assert(paint->micro[8]==7&&step_fill(paint,2)==FC_NOFILL&&step_locked(paint,8));
    for(uint32_t i=3;i<16;i++)assert(paint->step[i].time==ST_TIE);
    uint32_t same_locks=0;for(uint32_t i=0;i<NLOCK;i++)if(paint->lock[i].step==2&&paint->lock[i].param==P_LEVEL){same_locks++;assert(paint->lock[i].val==88);}
    assert(same_locks==1&&undo_swap(0)&&!memcmp(before_move,paint->step,sizeof before_move));
    assert(paint->micro[0]==-5&&paint->micro[2]==0&&step_locked(paint,0)&&step_locked(paint,2));
    assert(undo_swap(1)&&paint->step[2].n==3);
    /* Octave sweeps preserve intervals, attributes and ties, with one undo. */
    press(B_OCTDN);encs[panel.enc[EN_K2]]=1;frame();
    assert(paint->step[2].note[0]==72&&paint->step[2].note[1]==76&&paint->step[2].note[2]==79);
    encs[panel.enc[EN_K2]]=-2;frame();release(B_OCTDN);
    assert(paint->step[2].note[0]==48&&paint->step[2].vel==87&&paint->micro[2]==-5);
    assert(undo_swap(0)&&paint->step[2].note[0]==60);
    cursor_set(15);press(B_OCTUP);encs[panel.enc[EN_K2]]=100;frame();release(B_OCTUP);
    assert(paint->step[2].note[0]==120&&paint->step[2].note[2]==127&&ui.cursor==15);
    press(B_OCTUP);encs[panel.enc[EN_K2]]=1;frame();release(B_OCTUP);
    assert(paint->step[2].note[0]==120&&!strcmp(ui.msg,"OCTAVE LIMIT"));
    assert(undo_swap(0)&&paint->step[2].note[0]==60);
    /* Tie painting uses actual OCT+/STEP controls, crosses the 16-step bank,
     * preserves the source chord and unrelated step data, and has one undo. */
    steps_clear(paint);cursor_set(0);
    paint->p[P_SLEN]=32;
    for(uint32_t i=0;i<NSTEP;i++) step_clear(&paint->step[i]);
    paint->step[0].time=ST_NOTE;paint->step[0].n=3;
    paint->step[0].note[0]=60;paint->step[0].note[1]=64;paint->step[0].note[2]=67;
    paint->step[16].flags=SF_ACCENT;paint->step[16].lvl=90;
    step_t original[NSTEP];memcpy(original,paint->step,sizeof original);
    int8_t octave=song.octave;
    press(B_OCTUP);assert(song.octave==octave);
    encs[panel.enc[EN_K1]]=8;frame();
    encs[panel.enc[EN_K1]]=9;frame();
    assert(ui.cursor==17&&ui.bank==1);
    assert(!memcmp(&paint->step[0],&original[0],sizeof(step_t)));
    for(uint32_t i=1;i<=17;i++)assert(paint->step[i].time==ST_TIE);
    assert(paint->step[16].flags==SF_ACCENT&&paint->step[16].lvl==90);
    encs[panel.enc[EN_K1]]=-2;frame();assert(ui.cursor==15);
    assert(paint->step[17].time==ST_TIE);
    encs[panel.enc[EN_K1]]=100;frame();assert(ui.cursor==31);
    assert(paint->step[0].time==ST_NOTE&&paint->step[32].time==ST_REST);
    release(B_OCTUP);
    press(B_EDIT);tap(B_OCTDN);release(B_EDIT);
    assert(!memcmp(original,paint->step,sizeof original));
    press(B_EDIT);tap(B_OCTUP);release(B_EDIT);
    for(uint32_t i=1;i<32;i++)assert(paint->step[i].time==ST_TIE);
    frames(20); /* layer-release encoder quiet period (250 ms) */
    paint->step[5].time=ST_NOTE;
    cursor_set(4);press(B_OCTUP);encs[panel.enc[EN_K1]]=1;frame();release(B_OCTUP);
    assert(paint->step[5].time==ST_TIE);
    assert(undo_swap(0)); /* a new hold has its own snapshot */
    assert(paint->step[5].time==ST_NOTE&&paint->step[6].time==ST_TIE);
    cursor_set(4);press(B_OCTDN);assert(song.octave==octave);
    encs[panel.enc[EN_K1]]=3;frame();
    assert(paint->step[4].time==ST_TIE);
    for(uint32_t i=5;i<=7;i++)assert(paint->step[i].time==ST_REST);
    encs[panel.enc[EN_K1]]=-2;frame();assert(ui.cursor==5);
    assert(paint->step[7].time==ST_REST);
    release(B_OCTDN);assert(undo_swap(0));
    assert(paint->step[5].time==ST_NOTE&&paint->step[6].time==ST_TIE);
    assert(undo_swap(1));assert(paint->step[5].time==ST_REST);
    cursor_set(31);encs[panel.enc[EN_K1]]=1;frame();assert(ui.cursor==0);
    tap(B_OCTUP);assert(song.octave==octave&&ui.cursor==0);
    tap(B_ENV);tap(B_OCTUP);assert(song.octave==octave+1);
    tap(B_SEQ);expect_page("STEP");
    paint->p[P_SLEN]=16;cursor_set(0);
    select_turn(1);expect_page("PATTERN");
    select_turn(1);expect_page("RECORD");
    encs[panel.enc[EN_K1]]=1;frame();assert(rec_snap[1]==REC_SNAP_EIGHTH);
    assert(rec_snap[0]==REC_SNAP_TRACK&&paint->p[P_SDIV]==2);
    encs[panel.enc[EN_K1]]=-1;frame();assert(rec_snap[1]==REC_SNAP_TRACK);
    select_turn(1);expect_page("SEQUENCES");assert(sequence_browser);
    select_turn(1);expect_page("SEQUENCES"); /* shaping subpage */
    select_turn(1);expect_page("SEQUENCES"); /* pitch subpage */
    select_turn(1);expect_page("SONG");assert(!sequence_browser);
    select_turn(-1);expect_page("SEQUENCES");
    select_turn(-1);expect_page("RECORD");
    select_turn(-1);expect_page("PATTERN");
    select_turn(-1);expect_page("STEP");
    go_home();frame();
    static const char *const order[]={"STEP","PATTERN","RECORD","SEQUENCES","SONG","STEP"};
    for(uint32_t i=0;i<6;i++) {tap(B_SEQ);expect_page(order[i]);assert(song.sel==1);}
    select_turn(9);expect_page("SONG");tap(B_ENV);expect_page("ENV");
    tap(B_ENV);expect_page("ENV DEST");
    tap(B_SEQ);select_turn(9);expect_page("SONG");tap(B_LFO);expect_page("LFO");
    /* A sequence-derived chord sustains through fifteen ties while the actual
     * ARP MODE knob edits only track2, never the neighboring synths. */
    for(uint32_t i=0;i<NTRK;i++) steps_clear(&trk[i]);
    track_t *t=&trk[1];
    t->step[0].time=ST_NOTE;t->step[0].n=4;
    t->step[0].note[0]=60;t->step[0].note[1]=64;t->step[0].note[2]=67;t->step[0].note[3]=69;
    for(uint32_t i=1;i<16;i++)t->step[i].time=ST_TIE;
    t->p[P_SLEN]=16;t->p[P_AORDER]=AORDER_SEQ_NOTE;t->p[P_AOCT]=1;t->p[P_AMODE]=ARP_UP;
    trk[0].p[P_AMODE]=ARP_DOWN;trk[2].p[P_AMODE]=ARP_UPDOWN;
    seq_start();frame();assert(song.playing&&seq_harmony[1].n==4);
    tap(B_ARP);expect_page("ARP");frames(12); /* release debounce/encoder quiet */
    encs[panel.enc[EN_K1]]=-100;frame();assert(t->p[P_AMODE]==ARP_OFF);
    for(uint32_t mode=1;mode<ARP_COUNT;mode++) {
        frames(12);encs[panel.enc[EN_K1]]=1;frame();
        assert(t->p[P_AMODE]==(int16_t)mode);
        assert(trk[0].p[P_AMODE]==ARP_DOWN&&trk[2].p[P_AMODE]==ARP_UPDOWN);
        assert(song.sel==1&&song.playing);frame();
        assert(seq_harmony[1].n==4);
    }
    seq_stop();
    puts("native UI: tie/rest painting, banks, end stops, undo/redo; SEQ navigation; selected SNOTE track ARP modes while playing PASS");
    return 0;
}
