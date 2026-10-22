/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void knob(unsigned k,int amount){encs[panel.enc[EN_K1+k]]=amount;frame();}
int main(int argc,char **argv){
 outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
 song.sel=TRK_DRUM;go_home();open_family(FAM_FX);ui.page=page_first(FAM_FX);page_entered();frame();
 assert(cur_page()->graph==GR_FX&&page_for_drum(cur_page()));
 int16_t *value;assert(page_desc(cur_page(),2,&value)==&TP[P_DLY]&&value==&song.g[G_DRDLY]);
 assert(page_desc(cur_page(),3,&value)==&TP[P_REV]&&value==&song.g[G_DRREV]);
 int16_t synth_delay=trk[0].p[P_DLY],unused=TDRUM->p[P_DLY];
 song.g[G_DRDLY]=song.g[G_DRREV]=0;knob(2,200);knob(3,200);
 assert(song.g[G_DRDLY]==127&&song.g[G_DRREV]==127&&trk[0].p[P_DLY]==synth_delay&&TDRUM->p[P_DLY]==unused);
 knob(0,-200);assert(song.g[G_DRLVL]==0);knob(0,200);assert(song.g[G_DRLVL]==127);
 knob(1,-200);assert(TDRUM->p[P_PAN]==-64);knob(1,200);assert(TDRUM->p[P_PAN]==63);
 TDRUM->p[P_PAN]=0;ui.force=1;frame();ppm("drum-fx");
 /* Actual sends, through drums_mix; metronome and unrelated synths cannot
  * satisfy these assertions because we inspect the drum bus directly. */
 memset(&drums,0,sizeof drums);drum_factory_set=-2;thr_rev=thr_dly=0;
 TDRUM->p[P_E0]=0;TDRUM->att=0;TDRUM->p[P_MUTE]=0;
 mixacc_t mx;uint64_t reverb=0,delay=0;drum_on(LANE_NOTE[0],110);
 for(unsigned b=0;b<40;b++){memset(&mx,0,sizeof mx);mix_drums(CTL,&mx);for(unsigned i=0;i<CTL;i++){reverb+=abs(mx.sr[i]);delay+=abs(mx.sd[i]);}}
 assert(reverb>1000&&delay>1000);
 song.g[G_DRDLY]=song.g[G_DRREV]=0;memset(&mx,0,sizeof mx);mix_drums(CTL,&mx);
 for(unsigned i=0;i<CTL;i++)assert(!mx.sr[i]&&!mx.sd[i]);
 page_walk(1);frame();assert(cur_page()->graph==GR_FILTER&&page_for_drum(cur_page()));
 knob(0,-40);assert(TDRUM->p[P_TFLT]<0);ui.force=1;frame();ppm("drum-filter");
 for(unsigned p=0;p<NPAGES;p++)if(!strcmp(PAGES[p].title,"INSERT 2"))ui.page=p;
 page_entered();knob(1,1);assert(TDRUM->p[P_ITYPE]==1);
 knob(0,-200);assert(TDRUM->p[P_IMIX]==0);knob(2,200);assert(TDRUM->p[P_IA]==127);
 for(unsigned k=0;k<4;k++)assert(page_desc(cur_page(),k,&value)&&value);
 ui.force=1;frame();ppm("drum-insert-blend");
 track_select(0);for(unsigned k=0;k<4;k++)assert(page_desc(cur_page(),k,&value)&&value);
 puts("drum FX UI: real delay/reverb sends, level/pan, filter, track isolation and complete insert blend controls PASS");return 0;
}
