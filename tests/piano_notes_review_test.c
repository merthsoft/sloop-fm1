/* SPDX-License-Identifier: GPL-3.0-only */
/* Independent NOTES review. Optional `extreme-len` reproduces overflow. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <limits.h>
static void review_notes(void)
{
    seq_stop();fm1_in.buttons=fm1_in.notes=0;frames(2);go_home();song.sel=0;
    steps_clear(TSEL);TSEL->p[P_SLEN]=16;TSEL->p[P_VOICE]=V_POLY;
    TSEL->step[0]=(step_t){.time=ST_NOTE,.n=2,.note={60,64},.vel=90};
    TSEL->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={72},.vel=90};
    for(unsigned p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,"NOTES"))ui.page=p;
    ui.home=0;ui.cursor=0;page_entered();frame();
    assert(notes_page()&&ui.roll_pitch==60);
}
int main(int argc,char **argv)
{
    outdir="build/lofi-wet/review";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    review_notes();
    if(argc>1&&!strcmp(argv[1],"extreme-len")) {
        /* Actual NOTES edit route; length 1 plus INT_MAX overflows before clamp. */
        step_edit(2,INT_MAX);
        assert(note_chain_len(TSEL,0,60)==16); /* saturates at pattern end */
        return 0;
    }
    /* Pitch undo invalidates safely; redo restores the original identity. */
    step_edit(1,1);assert(ui.roll_pitch==61&&TSEL->step[0].note[0]==61);
    assert(undo_swap(0));assert(piano_selection_slot(TSEL,roll_selection())<0);
    step_t saved[NSTEP];memcpy(saved,TSEL->step,sizeof saved);
    step_edit(3,1);assert(!memcmp(saved,TSEL->step,sizeof saved));
    assert(undo_swap(1));assert(piano_selection_slot(TSEL,roll_selection())==0);
    step_edit(1,INT_MAX);assert(ui.roll_pitch==127);
    step_edit(1,INT_MIN);assert(ui.roll_pitch==1);
    step_edit(3,INT_MAX);assert((TSEL->step[0].lvl&3u)==LV_HARD);
    step_edit(3,INT_MIN);assert((TSEL->step[0].lvl&3u)==LV_GHOST);
    /* Empty cell operations are inert, including LEN and erase. */
    roll_select((piano_selection_t){8,0});memcpy(saved,TSEL->step,sizeof saved);
    step_edit(1,1);step_edit(2,1);step_edit(3,1);layer_tap(LY_ERASE);
    assert(!memcmp(saved,TSEL->step,sizeof saved));
    /* Continuations resolve to roots; individual LEN changes only one voice. */
    review_notes();assert(!note_extend(TSEL,0,3,1,1));
    roll_select(piano_selection_at(TSEL,2));assert(ui.cursor==0&&ui.roll_pitch==60);
    step_edit(2,-2);assert(note_chain_len(TSEL,0,60)==2&&note_chain_len(TSEL,0,64)==1);
    /* Page, track and project changes cannot retain an editable identity. */
    open_family(FAM_LFO);assert(!ui.roll_pitch);review_notes();
    track_select(1);assert(!ui.roll_pitch);track_select(0);assert(!ui.roll_pitch);
    roll_select((piano_selection_t){0,60});project_revision++;
    memcpy(saved,TSEL->step,sizeof saved);step_edit(2,1);assert(!memcmp(saved,TSEL->step,sizeof saved));
    /* Held OCT tool remains in charge; release requires fresh selection. */
    review_notes();press(B_OCTUP);step_edit(3,1);assert(ui.tie_voice==1);
    step_edit(0,2);assert(ui.cursor==2&&note_chain_len(TSEL,0,60)==3);
    release(B_OCTUP);assert(!ui.roll_pitch);memcpy(saved,TSEL->step,sizeof saved);
    step_edit(2,1);assert(!memcmp(saved,TSEL->step,sizeof saved));
    step_edit(0,INT_MIN);assert(ui.cursor==0&&ui.roll_pitch==60);
    step_edit(0,INT_MAX);assert(ui.cursor==15&&ui.roll_pitch==0);
    puts("NOTES independent review: undo/redo, identity invalidation, empty/continuation, held OCT and bounded navigation PASS");
    return 0;
}
