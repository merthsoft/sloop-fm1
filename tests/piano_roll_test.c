/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual input/renderer: viewport edits cannot mutate music or consume OCT tools. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
#include <assert.h>
static step_t saved_steps[NSTEP];
static int16_t saved_params[P_COUNT];
static uint16_t dirty_frame[240*240];
static void redraw_parity(void){
 memcpy(dirty_frame,screen,sizeof screen);ui.force=1;
 draw_frame();draw_head();draw_columns();draw_graph();draw_foot();ui.force=0;
 assert(!memcmp(dirty_frame,screen,sizeof screen));
}
static void turn(uint32_t encoder,int delta){encs[panel.enc[encoder]]=delta;frame();}
static void music_unchanged(void){assert(!memcmp(saved_steps,TSEL->step,sizeof saved_steps));assert(!memcmp(saved_params,TSEL->p,sizeof saved_params));}
static void fresh(void){
 seq_stop();fm1_in.buttons=fm1_in.notes=0;frames(20);song.sel=0;
 steps_clear(TSEL);TSEL->p[P_SLEN]=64;TSEL->p[P_VOICE]=V_POLY;
 TSEL->step[0]=(step_t){.note={60},.n=1,.vel=90};
 TSEL->step[8]=(step_t){.note={64},.n=1,.vel=100};
 TSEL->step[16]=(step_t){.note={67},.n=1,.vel=100};
 TSEL->step[24]=(step_t){.note={71},.n=1,.vel=100};
 assert(!note_extend(TSEL,0,63,0,1));
 memcpy(saved_steps,TSEL->step,sizeof saved_steps);memcpy(saved_params,TSEL->p,sizeof saved_params);
 go_home();open_family(FAM_SEQ);cursor_set(0);ui.roll_zoom=0;frame();
}
static void direct_roll(void){cv_begin(240,H_GRAPH,C_BLACK);cv_oy=0;graph_roll(TSEL,TE_COL[0]);}
static void continuous_c(void){
 direct_roll();int rows=0;
 for(unsigned y=20;y<118;y++){
  int joined=1;for(unsigned x=24;x<236;x++)if(cv_px[y*240+x]!=swap16(TE_COL[0])){joined=0;break;}
  rows+=joined;
 }
 assert(rows>0); /* C continues through attacks, grid lines and viewport boundaries. */
}
static void notes_checks(void){
 seq_stop();go_home();song.sel=0;steps_clear(TSEL);TSEL->p[P_SLEN]=16;TSEL->p[P_VOICE]=V_POLY;
 TSEL->step[0]=(step_t){.n=3,.note={60,64,67},.vel=90};
 TSEL->step[4]=(step_t){.n=1,.note={72},.vel=100};
 assert(!note_extend(TSEL,0,12,1,1));
 for(unsigned p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,"NOTES"))ui.page=p;
 ui.home=0;ui.cursor=0;page_entered();frame();assert(notes_page()&&ui.roll_pitch==60);
 memcpy(saved_steps,TSEL->step,sizeof saved_steps);
 turn(EN_K1,1);assert(ui.cursor==0&&ui.roll_pitch==64);
 turn(EN_K2,1);assert(TSEL->step[0].note[1]==65&&note_chain_len(TSEL,0,60)==13);
 turn(EN_K2,-1);assert(TSEL->step[0].note[1]==64&&note_chain_len(TSEL,0,60)==13);
 turn(EN_K3,3);assert(note_chain_len(TSEL,0,64)==4&&note_chain_len(TSEL,0,60)==13&&note_chain_len(TSEL,0,67)==1);
 turn(EN_K4,1);assert(((TSEL->step[0].lvl>>2)&3)==LV_HARD&&!(TSEL->step[0].lvl&3));
 ui.force=1;frame();ppm("notes-selected");redraw_parity();
 memcpy(saved_steps,TSEL->step,sizeof saved_steps);
 tap(B_EDIT);assert(note_slot(&TSEL->step[0],64)==TSEL->step[0].n&&note_chain_len(TSEL,0,60)==13);
 assert(undo_swap(0)&&!memcmp(saved_steps,TSEL->step,sizeof saved_steps));
 ui.roll_pitch=67;note_erase(TSEL,0,60);turn(EN_K4,1);
 int slot=piano_selection_slot(TSEL,roll_selection());assert(slot>=0&&((TSEL->step[0].lvl>>(slot*2))&3)==LV_HARD);
 memcpy(saved_steps,TSEL->step,sizeof saved_steps);project_revision++;turn(EN_K4,-1);
 assert(!memcmp(saved_steps,TSEL->step,sizeof saved_steps)&&!ui.roll_pitch);
 turn(EN_K1,1);assert(piano_selection_slot(TSEL,roll_selection())>=0);
 track_select(1);assert(!ui.roll_pitch);track_select(0);
 /* Unlinked pitch editing touches one voice, and refuses duplicate pitches. */
 steps_clear(TSEL);TSEL->step[0]=(step_t){.n=2,.note={60,64},.vel=90};roll_select((piano_selection_t){0,60});
 turn(EN_K2,2);assert(TSEL->step[0].note[0]==62&&TSEL->step[0].note[1]==64&&ui.roll_pitch==62);
 memcpy(saved_steps,TSEL->step,sizeof saved_steps);turn(EN_K2,2);assert(!memcmp(saved_steps,TSEL->step,sizeof saved_steps));
 assert(undo_swap(0)&&TSEL->step[0].note[0]==60);
 ui.force=1;frame();ppm("notes-chord");
}
int main(int argc,char **argv){
 outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);fresh();
 /* Graph starts below the cards; black-key rows and octave/beat rules are visible. */
 ui.force=1;frame();
 assert(screen[(Y_GRAPH+20)*240+22]==swap16(C_GRAY));
 int shaded=0;for(unsigned y=Y_GRAPH+20;y<Y_GRAPH+118;y++)
  shaded+=screen[y*240+30]==swap16(TE_G1);
 assert(shaded>0);
 assert(!strcmp(page_hold_hint(cur_page()),"PRESETS: ZOOM / OCT: TOOLS"));
 uint32_t undo_before=undo.sess;uint8_t valid=undo.valid;
 turn(EN_PRESET,-100);assert(ui.roll_zoom==-1&&roll_span()==8);music_unchanged();
 for(int z=-1;z<=2;z++){
  if(z!=-1)turn(EN_PRESET,1);
  assert(ui.roll_zoom==z&&roll_span()==(8u<<(z+1)));music_unchanged();continuous_c();
  redraw_parity();char name[32];snprintf(name,sizeof name,"roll-zoom-%u",roll_span());ppm(name);
 }
 turn(EN_PRESET,100);assert(ui.roll_zoom==2);assert(undo.sess==undo_before&&undo.valid==valid);music_unchanged();
 /* Window follows selection, independent of the 16-key input bank. */
 cursor_set(33);frame();turn(EN_PRESET,-100);assert(ui.bank==2&&roll_span()==8);continuous_c();
 turn(EN_PRESET,2);assert(roll_span()==32&&ui.bank==2);continuous_c();music_unchanged();
 /* An off-bank playhead matters in a 64-step view, not in a 16-step view. */
 cursor_set(0);ui.roll_zoom=0;song.playing=1;TSEL->seq_idx=40;
 uint32_t a=graph_signature();TSEL->seq_idx=41;assert(graph_signature()==a);
 ui.roll_zoom=2;a=graph_signature();TSEL->seq_idx=42;assert(graph_signature()!=a);
 song.playing=0;TSEL->seq_idx=0;
 /* Zoom keeps an anchored voice selection and an unmutated pattern while held. */
 press(B_OCTUP);turn(EN_K4,1);assert(ui.tie_start==0&&ui.tie_voice==1);
 turn(EN_PRESET,-1);assert(ui.tie_start==0&&ui.tie_voice==1);music_unchanged();
 turn(EN_K1,1);release(B_OCTUP);assert(note_chain_len(TSEL,0,60)==64);
 /* Short patterns have a safe nonzero viewport; every zoom stays bounded. */
 for(int len=1;len<=7;len++){
  TSEL->p[P_SLEN]=len;cursor_set(len-1);
  for(int z=-1;z<=2;z++){ui.roll_zoom=z;assert(roll_span()==(uint32_t)len);ui.force=1;frame();}
 }
 /* Zoom belongs to STEP, not sound editing or performance layers. */
 fresh();open_family(FAM_LFO);turn(EN_PRESET,1);assert(ui.roll_zoom==0);
 go_home();uint32_t sound=TSEL->preset+TSEL->eng_req*1000;turn(EN_PRESET,1);
 assert(sound!=TSEL->preset+TSEL->eng_req*1000&&ui.roll_zoom==0);
 notes_checks();
 puts("piano roll: 8/16/32/64 viewports, continuous voices, short patterns, scoped input, held tools and unchanged music PASS");return 0;
}
