/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);
    outdir="build/host";song.sel=1;go_home();frame();
    track_t *t=&trk[1];
    t->p[P_ROOT]=4;t->p[P_SCALE]=2;t->p[P_QUANT]=0;t->p[P_CHORD]=0;
    t->p[P_TRANS]=0;song.octave=0;
    char name[8];
    uint32_t f=key_of_white(0);
    assert(kb_map(t,f)==53); /* Literal F remains F for playing and erasing. */
    assert(layer_guide_note(t,f)==54);
    scale_note_name(name,t,layer_guide_note(t,f));assert(!strcmp(name,"F#3"));
    t->p[P_ROOT]=0;
    scale_note_name(name,t,layer_guide_note(t,key_of_white(6)));assert(!strcmp(name,"Eb4"));
    t->p[P_ROOT]=4;
    live_mod_t plot={.depth=127,.wave=3};
    assert(live_mod_plot_y(&plot,0,0)==21);
    assert(live_mod_plot_y(&plot,0x80000000u,0)==103);
    assert(live_mod_plot_y(&plot,0,1)==103);
    assert(live_mod_plot_y(&plot,0x80000000u,1)==20);
    plot.depth=0;
    assert(live_mod_plot_y(&plot,0,0)==62&&live_mod_plot_y(&plot,0,1)==20);
    for(int l=0;l<3;l++) {
        if(l<2) {
            live_mod[l].on=1;live_mod[l].part=1;live_mod[l].phase=0x40000000u;
            live_mod[l].value=l?32767:32767;
        }
        live_mod_t saved[2];memcpy(saved,live_mod,sizeof saved);
        ui.layer=l==0?LY_VIB:l==1?LY_TREM:LY_ERASE;ui.force=1;layer_screen_draw();
        assert(!memcmp(saved,live_mod,sizeof saved)); /* Drawing never advances the audio clock. */
        ppm(l==0?"guide-vibrato-em":l==1?"guide-tremolo-em":"guide-edit-em");
        if(l<2) {
            ly_lock=ui.layer;ui.force=1;layer_screen_draw();
            ppm(l==0?"vibrato-wave-locked":"tremolo-wave-locked");
            ly_lock=LY_PLAY;
        }
    }
    steps_clear(t);t->p[P_SLEN]=16;
    t->step[0]=(step_t){{53},1,ST_NOTE,0,100,0,0};
    t->step[1]=(step_t){{54},1,ST_NOTE,0,100,0,0};
    fm1_in.buttons=ly_bit[LY_ERASE];fm1_in.notes=1u<<f;frames(2);
    assert(!t->step[0].n&&t->step[1].n==1&&t->step[1].note[0]==54);
    fm1_in.buttons=0;fm1_in.notes=0;frames(30); /* pass the release-time knob guard */
    steps_clear(t);
    t->step[0]=(step_t){{60,64,67},3,ST_NOTE,0,100,0,0};
    t->step[1].time=ST_TIE;t->micro[0]=7;step_fill_set(t,0,FC_FILL);
    assert(lock_set(t,0,P_PAN,21));
    static track_t before,other;before=*t;other=trk[0];
    ui.step_sess=0;ui.layer=LY_ERASE;
    encs[panel.enc[EN_K4]]=-1;layer_knobs(LY_ERASE);
    assert(t->step[0].note[0]==48&&t->step[0].note[1]==52&&t->step[0].note[2]==55);
    assert(edit_delta.transpose==-12&&edit_delta.shift==0&&edit_delta.octave==-1);
    assert(t->step[1].time==ST_TIE&&t->micro[0]==7&&step_fill(t,0)==FC_FILL&&t->lock[0].step==0);
    encs[panel.enc[EN_K3]]=1;layer_knobs(LY_ERASE);assert(edit_delta.transpose==-11);
    encs[panel.enc[EN_K1]]=1;layer_knobs(LY_ERASE);assert(edit_delta.shift==1);
    assert(t->step[1].note[0]==49);
    ui.force=1;ui.msg_t=0;layer_screen_draw();ppm("edit-shift-transpose");
    assert(undo_swap(0));assert(!memcmp(t->step,before.step,sizeof t->step));
    assert(undo_swap(1));assert(t->step[1].note[0]==49&&edit_delta.transpose==-11);
    assert(undo_swap(0));
    encs[panel.enc[EN_K4]]=1;layer_knobs(LY_ERASE);
    assert(t->step[0].note[0]==72&&edit_delta.transpose==12&&edit_delta.shift==0&&edit_delta.octave==1);
    assert(!memcmp(&other,&trk[0],sizeof other));
    ui.step_sess=0;steps_clear(t);
    t->step[0]=(step_t){{5,17},2,ST_NOTE,0,100,0,0};
    pattern_transpose(t,-12);assert(t->step[0].note[0]==5&&t->step[0].note[1]==17);
    t->step[0].note[0]=115;t->step[0].note[1]=127;
    pattern_transpose(t,12);assert(t->step[0].note[0]==115&&t->step[0].note[1]==127);
    song.sel=TRK_DRUM;before=*TSEL;
    encs[panel.enc[EN_K4]]=-1;layer_knobs(LY_ERASE);assert(!memcmp(&before,TSEL,sizeof before));
    puts("EDIT tools: scale guide without remap, octave/semitone shifts, readout sessions, undo/redo, ties, bounds and track isolation PASS");
    return 0;
}
