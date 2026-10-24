/* Native controls, actual audio between UI frames, and complete pattern undo. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static track_t *t;
static void turn(uint32_t knob,int32_t amount) {encs[panel.enc[knob]]=amount;frame();}
static uint32_t slot(uint32_t at,uint32_t pitch)
{return note_slot(&t->step[at],pitch);}
static void fresh(void)
{
    seq_stop();fm1_in.buttons=fm1_in.notes=0;frames(20);
    song.sel=0;t=&trk[0];steps_clear(t);t->p[P_SLEN]=16;t->p[P_VOICE]=V_POLY;
    go_home();open_family(FAM_SEQ);ui.roll_zoom=0;cursor_set(0);frame();
}
static void roll_draw(void)
{
    cv_begin(240,124,C_BLACK);cv_oy=0;graph_roll(t,C_HI);
}
static void roll_pixel(uint32_t x,uint32_t pitch,uint32_t low)
{
    /* C4..B4 spans rows B3..C5 in the current roll: 14 rows, 7px high.
     * Sample the interior, away from grid lines and selection outlines. */
    uint32_t y=118-(pitch-low+2)*7+2;
    assert(cv_px[y*240+x]==swap16(C_HI));
}
static void display_audit(void)
{
    fresh();t->p[P_SLEN]=32;
    t->step[0]=(step_t){.note={60,64,67,71},.n=4};
    for(uint32_t i=1;i<32;i++) t->step[i].time=ST_TIE;
    /* Legacy TIE/REST payload is not audible, and must not change vertical scale. */
    t->step[10].n=1;t->step[10].note[0]=127;
    t->step[31]=(step_t){.note={1},.n=1,.time=ST_REST};
    roll_draw();
    for(uint32_t k=0;k<4;k++) {
        roll_pixel(24,t->step[0].note[k],60);
        for(uint32_t x=23;x<237;x++) roll_pixel(x,t->step[0].note[k],60);
    }
    cursor_set(16);roll_draw();
    for(uint32_t k=0;k<4;k++) roll_pixel(22,t->step[0].note[k],60);
    /* Mixed independent continuations share exactly the same connected geometry. */
    t->step[17]=(step_t){.note={60,64,67,71},.n=4,.flags=SF_TIES};
    roll_draw();for(uint32_t k=0;k<4;k++) roll_pixel(40,t->step[0].note[k],60);
    /* Every chord slot invalidates cached graph pixels, without forcing a redraw. */
    settings.zoom=0;ui.force=1;draw_graph();
    uint16_t before[240*240];memcpy(before,screen,sizeof before);
    uint32_t old=ui.graph_sig;
    t->step[17].note[2]=68;draw_graph();
    assert(ui.graph_sig!=old && memcmp(before,screen,sizeof before));
    memcpy(before,screen,sizeof before);old=ui.graph_sig;
    t->step[17].note[3]=70;draw_graph();
    assert(ui.graph_sig!=old && memcmp(before,screen,sizeof before));
    memcpy(before,screen,sizeof before);ui.force=1;draw_graph();
    assert(!memcmp(before,screen,sizeof before));
    /* Knob zoom must not temporarily hide the upper voices of the STEP roll. */
    settings.zoom=1;ui.hot_t=30;ui.hot_col=1;ui.force=1;draw_columns();draw_graph();
    uint32_t y=Y_GRAPH+118-(71-60+2)*7+2;
    assert(screen[y*240+24]==swap16(TE_COL[0]));
    settings.zoom=0;
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    display_audit();
    fresh();
    t->step[0]=(step_t){.note={60},.n=1,.vel=82};
    t->step[1]=(step_t){.note={64},.n=1,.vel=91,.lvl=2,.rat=1,.flags=SF_ACCENT};
    t->step[2]=(step_t){.note={67},.n=1,.vel=101};
    t->micro[1]=7;step_fill_set(t,1,FC_NOFILL);assert(lock_set(t,1,P_LEVEL,70));
    step_t original[NSTEP];memcpy(original,t->step,sizeof original);
    press(B_OCTUP);turn(EN_K1,3);
    assert(ui.cursor==3 && t->step[1].note[0]==64 && t->step[1].vel==91);
    assert(t->step[1].n==2 && (t->step[1].flags&SF_TIE(slot(1,60))));
    assert(t->step[1].flags&SF_ACCENT && t->step[1].lvl==2 && t->step[1].rat==1);
    assert(t->micro[1]==7 && step_fill(t,1)==FC_NOFILL && step_locked(t,1));
    assert(t->step[2].note[0]==67 && (t->step[2].flags&SF_TIE(slot(2,60))));
    outdir="build/host";ui.force=1;frame();ppm("per-note-ties-all");
    frames(150);assert(ui.msg_t && !strcmp(ui.msg,"HOLD 1 TIE 4 VOICE"));
    turn(EN_K1,-2);assert(ui.cursor==1 && slot(3,60)<t->step[3].n);
    release(B_OCTUP);assert(undo_swap(0) && !memcmp(original,t->step,sizeof original));
    assert(undo_swap(1) && slot(3,60)<t->step[3].n);

    /* Source stays step 2 while the cursor crosses G. Select only E, not carried C. */
    cursor_set(1);press(B_OCTUP);turn(EN_K4,1);assert(ui.tie_voice==1);
    turn(EN_K1,2);release(B_OCTUP);
    assert(t->step[2].n==3 && (t->step[2].flags&SF_TIE(slot(2,64))));
    assert(t->step[3].n==2 && (t->step[3].flags&SF_TIE(slot(3,64))));
    ui.force=1;frame();ppm("per-note-ties-voice");
    turn(EN_K3,-1);
    assert(slot(3,64)==t->step[3].n && slot(3,60)<t->step[3].n);
    assert(slot(2,64)<t->step[2].n && t->step[2].note[0]==67);
    assert(undo_swap(0) && slot(3,64)<t->step[3].n);assert(undo_swap(1));

    /* EDIT removes only the selected voice and its continuations; undo restores all. */
    cursor_set(1);press(B_OCTUP);turn(EN_K4,1);release(B_OCTUP);
    memcpy(original,t->step,sizeof original);tap(B_EDIT);
    assert(slot(1,64)==t->step[1].n && slot(2,64)==t->step[2].n);
    assert(slot(1,60)<t->step[1].n && t->step[2].note[0]==67);
    assert(undo_swap(0) && !memcmp(original,t->step,sizeof original));

    /* Refuse an entire increment at capacity or a deliberate same-pitch reattack. */
    fresh();t->step[0]=(step_t){.note={60},.n=1};
    t->step[1]=(step_t){.note={64},.n=1};
    t->step[2]=(step_t){.note={65,67,69,71},.n=4};
    memcpy(original,t->step,sizeof original);
    press(B_OCTUP);turn(EN_K1,2);
    assert(ui.cursor==0 && !memcmp(original,t->step,sizeof original) && !strcmp(ui.msg,"CHORD FULL"));
    release(B_OCTUP);t->step[2]=(step_t){.note={60},.n=1};memcpy(original,t->step,sizeof original);
    press(B_OCTUP);turn(EN_K1,2);
    assert(ui.cursor==0 && !memcmp(original,t->step,sizeof original) && !strcmp(ui.msg,"NOTE AHEAD"));
    release(B_OCTUP);

    /* NOTE/LEN keep describing the anchor, even after shortening beyond the cursor. */
    fresh();t->step[0]=(step_t){.note={60},.n=1};
    t->step[1]=(step_t){.note={64},.n=1};
    press(B_OCTUP);turn(EN_K1,3);release(B_OCTUP);
    assert(!strcmp(ui.big_v[1],"C4") && !strcmp(ui.big_u[1],"SRC"));
    turn(EN_K3,-2);assert(t->step[3].time==ST_REST);
    assert(!strcmp(ui.big_l[2],"LEN") && !strcmp(ui.big_v[2],"2"));
    turn(EN_K3,1);assert(note_chain_len(t,0,60)==3);
    cursor_set(3);frame();assert(!strcmp(ui.big_v[1],"--"));

    /* Existing whole ties are expanded only where another voice needs to end. */
    fresh();t->step[0]=(step_t){.note={60,64},.n=2};
    t->step[1].time=t->step[2].time=ST_TIE;t->step[3]=(step_t){.note={67},.n=1};
    press(B_OCTUP);turn(EN_K4,2);turn(EN_K1,4);release(B_OCTUP);
    assert(t->step[1].time==ST_TIE && t->step[2].time==ST_TIE);
    assert(t->step[3].n==2 && (t->step[3].flags&SF_TIE(slot(3,64))));
    turn(EN_K3,-3);
    assert(t->step[2].n==1 && t->step[2].note[0]==60);
    assert(t->step[3].n==1 && t->step[3].note[0]==67 && t->step[4].time==ST_REST);

    /* Reverse erase compacts all slot metadata and clears following continuations. */
    fresh();t->step[0]=(step_t){.note={60,64,67},.n=3,.lvl=0x39,.rat=0x24};
    t->step[1]=(step_t){.note={60,64,67},.n=3,.flags=SF_TIE(0)|SF_TIE(1)|SF_TIE(2)};
    er_notes[2]=1u;erase_step(t,0);er_notes[2]=0; /* MIDI 64 */
    assert(t->step[0].n==2 && t->step[0].note[1]==67 && (t->step[0].lvl&15)==13);
    assert(t->step[1].n==2 && t->step[1].note[1]==67 && (t->step[1].flags&SF_TIE(1)));

    /* Structural pitch/move gestures cannot leave detached continuations. */
    cursor_set(0);press(B_OCTUP);turn(EN_K2,1);turn(EN_K3,1);release(B_OCTUP);
    assert(t->step[0].note[0]==60 && ui.cursor==0);
    puts("native note ties: overlapping attacks, selected voice, shorten/delete, atomic refusal, metadata and undo PASS");
    return 0;
}
