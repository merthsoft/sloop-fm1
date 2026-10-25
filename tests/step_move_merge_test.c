/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void fresh(void)
{
    host_tracks_init();song.sel=1;song.playing=song.rec=transport_req=rec_wait=ft_on=0;
    steps_clear(TSEL);ui.cursor=4;ui.tie_sess=(undo_sess+=4u)|3u;
}
int main(void)
{
    panel=PANEL_DEFAULT;layers_init();palette_set(4);outdir="build/host";
    fresh();open_family(FAM_SEQ);assert(cur_page()->scope==SC_STEP);
    for(uint32_t b=B_OCTDN;b<=B_OCTUP;b++) {
        press(b);frames(180);assert(ui.msg_t&&!memcmp(ui.msg,"HOLD 1 ",7));
        ui_message("CHORD FULL");frames(10);assert(ui.msg_t&&!strcmp(ui.msg,"CHORD FULL"));
        frames(60);assert(ui.msg_t&&!memcmp(ui.msg,"HOLD 1 ",7));
        release(b);assert(!ui.msg_t);
    }
    press(B_OCTUP);go_home();frame();assert(!ui.msg_t);release(B_OCTUP);
    ui_message("ORDINARY MESSAGE");frames(60);assert(!ui.msg_t);
    fresh();track_t *t=TSEL;
    t->step[3]=(step_t){.time=ST_NOTE,.n=2,.note={60,64},.vel=91,.rat=2};
    t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67},.vel=50};t->micro[4]=-7;
    t->micro[3]=2;step_fill_set(t,3,FC_FILL);lock_set(t,3,P_DIST,12);
    step_t before[64];memcpy(before,t->step,sizeof before);
    step_move(-1);
    assert(ui.cursor==3&&t->step[3].n==3&&t->step[3].note[2]==67&&t->step[3].vel==91&&t->step[3].rat==2);
    assert(t->step[4].time==ST_REST&&t->micro[4]==0&&t->micro[3]==2&&step_fill(t,3)==FC_FILL&&lock_find(t,3,P_DIST,0)>=0);
    assert(undo_swap(0)&&!memcmp(before,t->step,sizeof before));assert(undo_swap(1)&&t->step[3].n==3);
    fresh();t=TSEL;t->step[3]=(step_t){.time=ST_NOTE,.n=4,.note={60,64,67,71}};
    t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={74}};memcpy(before,t->step,sizeof before);uint32_t session=undo.sess;
    step_move(-1);assert(!memcmp(before,t->step,sizeof before)&&ui.cursor==4&&undo.sess==session);
    t->step[4].note[0]=67;step_move(-1);assert(t->step[3].n==4&&t->step[4].time==ST_REST);
    fresh();t=TSEL;t->step[2]=(step_t){.time=ST_NOTE,.n=1,.note={60}};t->step[3].time=ST_TIE;
    t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67}};t->step[5].time=ST_TIE;
    step_move(-2);assert(t->step[2].n==2&&t->step[3].time==ST_TIE&&t->step[4].time==ST_REST&&t->step[5].time==ST_REST);
    fresh();t=TSEL;t->step[3]=(step_t){.time=ST_NOTE,.n=1,.note={60}};
    t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67}};t->step[5].time=ST_TIE;memcpy(before,t->step,sizeof before);
    step_move(-1);assert(!memcmp(before,t->step,sizeof before)&&ui.cursor==4);
    fresh();t=TSEL;t->step[3]=(step_t){.time=ST_NOTE,.n=1,.note={60}};t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67}};lock_set(t,4,P_DIST,20);
    memcpy(before,t->step,sizeof before);step_move(-1);assert(!memcmp(before,t->step,sizeof before)&&lock_find(t,4,P_DIST,0)>=0);
    /* The REST brush changes time only; stale note fields are deliberately retained. */
    fresh();t=TSEL;
    t->step[3]=(step_t){.time=ST_REST,.n=2,.note={60,64},.vel=90,.flags=3,.lvl=7,.rat=2};
    t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67},.vel=80};
    t->micro[3]=-9;step_fill_set(t,3,FC_FILL);
    memcpy(before,t->step,sizeof before);
    step_move(-1);assert(!t->micro[3]&&!step_fill(t,3));assert(ui.cursor==3&&t->step[3].time==ST_NOTE&&t->step[3].note[0]==67&&t->step[4].time==ST_REST);
    assert(undo_swap(0)&&!memcmp(before,t->step,sizeof before));
    assert(undo_swap(1)&&t->step[3].note[0]==67);
    /* Silent cells with deliberate automation must still survive failed moves. */
    for(uint32_t guard=0;guard<1;guard++) {
        fresh();t=TSEL;t->step[4]=(step_t){.time=ST_NOTE,.n=1,.note={67}};
        lock_set(t,3,P_DIST,20);
        memcpy(before,t->step,sizeof before);session=undo.sess;
        step_move(-1);assert(ui.cursor==4&&!memcmp(before,t->step,sizeof before)&&undo.sess==session);
    }
    puts("STEP help: held lifetime, warning priority and release/navigation clearing PASS\nmove merge: chord union, duplicate/full rejection, destination metadata, complete undo/redo, ties and lock guards PASS");return 0;
}
