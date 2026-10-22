/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual panel/renderer regressions for staged native action sheets.
 * Run individual cases by name to retain independent failure traces. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <limits.h>
static struct {step_t step[NSTEP];int8_t micro[NSTEP];uint8_t fill[NSTEP/4];} music;
static uint16_t before_screen[240*240];
static void remember(void){memcpy(music.step,TSEL->step,sizeof music.step);memcpy(music.micro,TSEL->micro,sizeof music.micro);memcpy(music.fill,TSEL->fill,sizeof music.fill);}
static void unchanged(void){assert(!memcmp(music.step,TSEL->step,sizeof music.step));assert(!memcmp(music.micro,TSEL->micro,sizeof music.micro));assert(!memcmp(music.fill,TSEL->fill,sizeof music.fill));}
static void turn(uint32_t e,int32_t d){encs[panel.enc[e]]=d;frame();}
static void parity(void){memcpy(before_screen,screen,sizeof screen);ui.force=1;actions_draw();ui.force=0;assert(!memcmp(before_screen,screen,sizeof screen));}
static void row(unsigned n){turn(EN_K1,(int32_t)n-note_action.row);assert(note_action.row==n);parity();}
static void undo_input(void){press(B_EDIT);tap(B_OCTDN);release(B_EDIT);frames((LY_QUIET_MS+15u)/16u+1u);assert(!ly_quiet());}
static void fresh(void)
{
    seq_stop();song.rec=rec_wait=ft_on=0;transport_req=0;fm1_in.buttons=fm1_in.notes=0;
    frames(20);go_home();track_select(0);ui.menu=ui.confirm=0;steps_clear(TSEL);
    TSEL->p[P_SLEN]=16;TSEL->p[P_VOICE]=V_POLY;
    TSEL->step[0]=(step_t){.time=ST_NOTE,.n=2,.note={60,64},.vel=90};
    assert(!note_extend(TSEL,0,2,2,1));
    TSEL->step[8]=(step_t){.time=ST_NOTE,.n=1,.note={64},.vel=90};
    for(unsigned p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,"NOTES"))ui.page=p;
    undo.valid=0;ui.home=0;ui.cursor=0;ui.note_context=0;page_entered();frame();
    turn(EN_K1,1);assert(ui.roll_pitch==64&&ui.cursor==0);
}
static void open_actions(void){turn(EN_SELECT,1);assert(actions_on()&&actions_ready()&&ui.roll_pitch==64&&note_action.at==0&&kb_grid==2);parity();}
static void identity(void)
{
    fresh();remember();open_actions();turn(EN_SELECT,-1);
    assert(notes_page()&&ui.roll_pitch==64);unchanged();open_actions();
    tap(B_OCTDN);assert(notes_page()&&ui.roll_pitch==64&&song.octave==0);unchanged();
}
static void values(void)
{
    fresh();open_actions();assert(strstr(NOTE_ACTIONS[0],"STEP")&&strstr(NOTE_ACTIONS[1],"STEP")&&strstr(NOTE_ACTIONS[2],"STEP")&&strstr(NOTE_ACTIONS[3],"STEP"));
    row(0);turn(EN_K2,1);assert(TSEL->step[0].flags&SF_SLIDE);parity();
    row(1);turn(EN_K2,INT_MAX);assert(TSEL->micro[0]==MICRO_MAX);parity();
    row(2);turn(EN_K2,1);assert(step_fill(TSEL,0)==1);parity();
    row(3);turn(EN_K2,1);assert(TSEL->step[0].flags&SF_ACCENT);parity();
    remember();turn(EN_K3,20);turn(EN_K4,20);unchanged();
    row(6);turn(EN_K1,INT_MAX);assert(note_action.row==6);turn(EN_K1,INT_MIN);assert(!note_action.row);
}
static void copy_move(void)
{
    fresh();open_actions();row(5);remember();turn(EN_K2,4);unchanged();assert(note_action.dest==4);
    press(B_OCTUP);assert(note_chain_len(TSEL,4,64)==3&&note_chain_len(TSEL,0,64)==3);parity();
    uint32_t session=undo.sess;remember();frames(5);assert(undo.sess==session);unchanged();release(B_OCTUP);
    undo_input();assert(note_slot(&TSEL->step[4],64)==TSEL->step[4].n&&note_chain_len(TSEL,0,64)==3);
    assert(actions_on());row(4);remember();turn(EN_K2,1);assert(note_action.dest==5);unchanged();
    tap(B_OCTUP);assert(note_chain_len(TSEL,5,64)==3&&note_slot(&TSEL->step[0],64)==TSEL->step[0].n);
    assert(TSEL->step[0].note[0]==60&&ui.cursor==5&&ui.roll_pitch==64&&note_action.at==5);parity();
    undo_input();assert(note_chain_len(TSEL,0,64)==3&&note_slot(&TSEL->step[5],64)==TSEL->step[5].n);
}
static void delete_note(void)
{
    fresh();open_actions();row(6);tap(B_OCTUP);
    assert(note_slot(&TSEL->step[0],64)==TSEL->step[0].n&&TSEL->step[0].note[0]==60&&!ui.roll_pitch);
    assert(!actions_ready());parity();undo_input();assert(note_chain_len(TSEL,0,64)==3);
}
static void occupied(void)
{
    fresh();open_actions();row(5);turn(EN_K2,8);remember();uint32_t session=undo.sess;
    tap(B_OCTUP);unchanged();assert(undo.sess==session);assert(strstr(ui.msg,"ALREADY"));
}
static void exit_knob(void)
{
    fresh();open_actions();remember();encs[panel.enc[EN_SELECT]]=-1;encs[panel.enc[EN_K2]]=1;frame();
    assert(notes_page()&&ui.roll_pitch==64);unchanged();frame();unchanged();
}
static void keyboard(void)
{
    fresh();open_actions();remember();uint32_t gates=chord_gates();key(0);key(4);unchanged();assert(chord_gates()==gates&&kb_grid==2);
    assert(ui.roll_pitch==64&&note_action.at==0);
}
static void priority(void)
{
    fresh();open_actions();row(5);turn(EN_K2,4);remember();
    press(B_EDIT);frames(SHOW_MS/16u+2u);assert(ui.layer==LY_ERASE);tap(B_OCTUP);release(B_EDIT);frames((LY_QUIET_MS+15u)/16u+1u);
    /* EDIT+OCT+ is redo, never APPLY the staged copy. */
    unchanged();
}
static void transport(void)
{
    fresh();open_actions();remember();tap(B_PLAY);assert(!actions_on());unchanged();
    seq_stop();transport_req=0;fresh();open_actions();remember();press(B_REC);assert(!actions_on());unchanged();release(B_REC);
}
static void invalidation(void)
{
    fresh();open_actions();remember();project_revision++;assert(!actions_ready());turn(EN_K2,1);tap(B_OCTUP);unchanged();
    fresh();open_actions();turn(EN_ALGO,1);assert(song.sel==1&&!actions_on()&&!ui.roll_pitch);
    fresh();roll_select((piano_selection_t){12,0});turn(EN_SELECT,1);assert(actions_on()&&!actions_ready());
    remember();turn(EN_K2,1);row(6);tap(B_OCTUP);unchanged();parity();
}
static void open_drum_actions(void)
{
    fresh();track_select(TRK_DRUM);steps_clear(TSEL);TSEL->p[P_SLEN]=16;
    drum_lane=2;drum_cursor=1;dstep_set(&TSEL->dstep[1],2,LV_SOFT,1);dstep_set(&TSEL->dstep[1],3,LV_HARD,0);
    studio_open(SC_DRUM);drum_page=4;frame();turn(EN_SELECT,1);
    assert(actions_on()&&note_action.hit&&note_action.lane==2&&note_action.at==1&&actions_ready()&&kb_grid==2);parity();
}
static void drum_actions(void)
{
    open_drum_actions();
    assert(strstr(HIT_ACTIONS[1],"STEP")&&strstr(HIT_ACTIONS[2],"STEP"));
    row(0);turn(EN_K2,1);assert(dstep_rat(&TSEL->dstep[1],2)==2);parity();
    row(4);remember();turn(EN_K2,3);unchanged();tap(B_OCTUP);
    assert(dstep_has(&TSEL->dstep[4],2)&&dstep_rat(&TSEL->dstep[4],2)==2&&dstep_lvl(&TSEL->dstep[4],2)==LV_SOFT);
    undo_input();assert(!dstep_has(&TSEL->dstep[4],2)&&dstep_has(&TSEL->dstep[1],2));
    row(3);tap(B_OCTUP);assert(!dstep_has(&TSEL->dstep[1],2)&&dstep_has(&TSEL->dstep[1],3)&&note_action.at==4);
    undo_input();assert(dstep_has(&TSEL->dstep[1],2)&&!dstep_has(&TSEL->dstep[4],2));
    /* Reopen the source after undo invalidates the moved identity. */
    tap(B_OCTDN);assert(drum_page==0&&!actions_on());drum_cursor=1;drum_page=4;frame();turn(EN_SELECT,1);
    row(5);tap(B_OCTUP);assert(!dstep_has(&TSEL->dstep[1],2)&&dstep_has(&TSEL->dstep[1],3));
    undo_input();assert(dstep_has(&TSEL->dstep[1],2));
    tap(B_OCTDN);assert(drum_page==0&&!actions_on());
}
static void drum_edges(void)
{
    open_drum_actions();remember();uint32_t age=drums.age;key(0);key(4);unchanged();assert(drums.age==age&&kb_grid==2);
    row(4);turn(EN_K2,3);dstep_set(&TSEL->dstep[4],2,LV_NORM,0);remember();uint32_t session=undo.sess;
    tap(B_OCTUP);unchanged();assert(undo.sess==session&&strstr(ui.msg,"ALREADY"));
    dstep_clr(&TSEL->dstep[4],2);press(B_OCTUP);assert(dstep_has(&TSEL->dstep[4],2));session=undo.sess;
    remember();frames(5);unchanged();assert(undo.sess==session);release(B_OCTUP);
    encs[panel.enc[EN_SELECT]]=-1;encs[panel.enc[EN_K2]]=1;remember();frame();
    assert(drum_page==4&&!actions_on());unchanged();frame();unchanged();
    turn(EN_SELECT,1);remember();project_revision++;turn(EN_K2,1);tap(B_OCTUP);unchanged();assert(!actions_ready());
}
int main(int argc,char **argv)
{
    outdir="build/lofi-wet/review";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    struct {const char *name;void (*run)(void);} cases[]={
        {"identity",identity},{"values",values},{"copy-move",copy_move},{"delete",delete_note},{"occupied",occupied},
        {"exit-knob",exit_knob},{"keyboard",keyboard},{"priority",priority},{"transport",transport},{"invalidation",invalidation},{"drums",drum_actions},{"drum-edges",drum_edges}};
    for(unsigned i=0;i<NELEM(cases);i++)if(argc<2||!strcmp(argv[1],cases[i].name)){printf("actions UI: %s\n",cases[i].name);fflush(stdout);cases[i].run();}
    puts("actions UI: requested assertions PASS");return 0;
}
