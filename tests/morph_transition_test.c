/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void store(unsigned slot){ui.arm=0;snapshot_edit(slot,1);snapshot_edit(slot,1);}
static void clock_at(uint32_t units){clk_beat=units/BEAT_U;clk_pos=units%BEAT_U;}
static void endpoints(void) {
 host_tracks_init();song.sel=0;memset(snapshot_p,0,sizeof snapshot_p);memset(snapshot_run,0,sizeof snapshot_run);
 host_preset(&trk[0],0,0);trk[0].p[P_E0]=0;trk[0].p[P_E4]=30;trk[0].p[P_PAN]=-40;
 store(1);trk[0].p[P_E4]=100;trk[0].p[P_PAN]=40;store(2);
}
int main(int argc,char **argv) {
 outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);knob_accel_off=1;
 /* All scopes on all engines preserve every unselected parameter. */
 for(unsigned e=0;e<NENGINES;e++)for(unsigned scope=0;scope<NELEM(N_MORPH_SCOPE);scope++) {
  host_preset(&trk[0],e,0);track_t *t=&trk[0];memset(snapshot_p,0,sizeof snapshot_p);
  int16_t before[P_COUNT];memcpy(before,t->p,sizeof before);
  for(unsigned id=0;id<P_COUNT;id++) {
   const param_desc_t *d=track_desc(t,id);
   snapshot_values[0][0][id]=d->min;snapshot_values[0][1][id]=d->max;
  }
  snapshot_p[0][3]=3;snapshot_p[0][4]=(int16_t)scope;
  uint32_t rng=rng_state,midi=mo_w;
  snapshot_apply(t,127,0);
  for(unsigned id=0;id<P_COUNT;id++) {
   int selected=snapshot_param(id)&&snapshot_selected(t,id);
   assert(t->p[id]==(selected?snapshot_values[0][1][id]:before[id]));
  }
  assert(rng==rng_state&&midi==mo_w);
 }
 /* Meaningful category examples, rather than only checking the classifier. */
 endpoints();snapshot_p[0][4]=3;snapshot_apply(&trk[0],0,0);
 assert(trk[0].p[P_PAN]==40&&trk[0].p[P_E4]==100);
 snapshot_p[0][4]=2;snapshot_apply(&trk[0],0,0);
 assert(trk[0].p[P_E4]==30&&trk[0].p[P_PAN]==40);
 snapshot_p[0][4]=5;snapshot_apply(&trk[0],0,0);assert(trk[0].p[P_PAN]==-40);
 /* Every free timing, both directions, completes exactly once and holds. */
 for(unsigned rate=0;rate<8;rate++)for(unsigned dir=1;dir<=2;dir++) {
  endpoints();snapshot_p[0][6]=(int16_t)rate;snapshot_edit(5,(int32_t)dir);
  uint64_t blocks=(UINT64_C(1)<<32)/motion_step(rate)+1u;
  int previous=snapshot_p[0][0];
  for(uint64_t b=0;b<blocks;b++) {
   snapshot_tick(&trk[0]);int p=snapshot_p[0][0];
   assert(dir==1?p>=previous:p<=previous);
   if(b+1u<blocks)assert(p!=(dir==1?127:0));
   previous=p;
  }
  assert(!snapshot_p[0][5]&&snapshot_p[0][0]==(dir==1?127:0));
  int16_t final[P_COUNT];memcpy(final,trk[0].p,sizeof final);
  for(unsigned b=0;b<50;b++)snapshot_tick(&trk[0]);assert(!memcmp(final,trk[0].p,sizeof final));
 }
 /* Synced durations, pause, transport reset and resume. */
 for(unsigned rate=8;rate<NELEM(N_EVOLVE_CYCLE);rate++) {
  endpoints();snapshot_p[0][6]=(int16_t)rate;song.playing=1;clock_at(0);snapshot_edit(5,1);
  uint32_t period=(EVOLVE_UNITS[rate-8]/8u)*(BEAT_U/12u);
  clock_at(period/4);snapshot_tick(&trk[0]);assert(snapshot_p[0][0]==31);
  song.playing=0;clock_at(0);snapshot_tick(&trk[0]);assert(snapshot_p[0][0]==31);
  song.playing=1;snapshot_tick(&trk[0]);assert(snapshot_p[0][0]==31);
  for(unsigned i=1;i<=3;i++){clock_at(i*period/4);snapshot_tick(&trk[0]);}
  assert(!snapshot_p[0][5]&&snapshot_p[0][0]==127);
 }
 /* STOP retains the current value; manual/capture cancel and DRIFT yields. */
 endpoints();drift_p[0][0]=2;drift_p[0][3]=DRIFT_MORPH;drift[0].target=DRIFT_MORPH;drift[0].depth=123;
 snapshot_edit(5,1);assert(!drift_p[0][0]&&!drift[0].depth);
 for(unsigned b=0;b<500;b++)snapshot_tick(&trk[0]);int mid=snapshot_p[0][0];
 snapshot_edit(5,-1);snapshot_tick(&trk[0]);assert(snapshot_p[0][0]==mid&&!snapshot_p[0][5]);
 snapshot_edit(5,1);snapshot_edit(0,3);assert(!snapshot_p[0][5]);
 snapshot_edit(5,1);store(1);assert(!snapshot_p[0][5]);
 /* Source safety depends on scope, and timing changes retain the core-0 guard. */
 endpoints();host_preset(&trk[0],ENGI_GRAIN,0);snapshot_p[0][3]=3;
 snapshot_values[0][0][P_E0]=0;snapshot_values[0][1][P_E0]=1;
 snapshot_edit(5,1);assert(!snapshot_p[0][5]);snapshot_p[0][4]=3;snapshot_edit(5,1);assert(snapshot_p[0][5]);
 endpoints();snapshot_p[0][5]=1;cpu1_units=1;
 snapshot_values[0][1][P_LRATE]++;assert(!(units_split()&1u));
 snapshot_p[0][4]=1;assert(snapshot_lfo_stable(0,trk[0].p));
 /* Real mixer commits the moving base and matches a manual apply exactly. */
 endpoints();track_t *t=&trk[0];t->pold=t->xf_on=0;duck.g0=duck.g1=32767;
 trk_note_on(t,60,100);snapshot_edit(5,1);snapshot_run[0].phase=0x40000000u;
 track_t base=*t;uint32_t midi=mo_w,random=rng_state;
 memset(&acc[0],0,sizeof acc[0]);mix_part(t,CTL,&acc[0],part_buf[0]);
 __typeof__(acc[0]) automatic=acc[0];int position=snapshot_p[0][0];
 assert(position>0&&position<127&&t->p[P_E4]>30&&t->p[P_E4]<100);
 assert(mo_w==midi&&rng_state==random);
 *t=base;snapshot_p[0][5]=0;snapshot_apply(t,position,0);
 memset(&acc[0],0,sizeof acc[0]);mix_part_run(t,CTL,&acc[0],part_buf[0]);
 assert(!memcmp(&automatic,&acc[0],sizeof automatic));
 /* Actual page-to-parameter mapping, including non-column-zero MORPH readout. */
 endpoints();ui.home=0;open_family(FAM_EDIT);
 for(unsigned i=0;i<NPAGES&&cur_page()->scope!=SC_MORPH;i++)page_walk(1);
 edit_param(3,2);assert(snapshot_p[0][4]==2);ui.msg_t=ui.hot_t=0;ui.force=1;frame();ppm("morph-scope");
 page_walk(1);assert(cur_page()->id[0]==5);edit_param(1,8);assert(snapshot_p[0][6]==8);
 edit_param(0,1);assert(snapshot_p[0][5]==1);ui.force=1;frame();ppm("morph-go");
 puts("MORPH scope and one-shot: category isolation, all durations/directions, stop/resume, endpoint hold, source/LFO safety and UI PASS");return 0;
}
