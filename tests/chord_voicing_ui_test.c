/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static void turn(uint32_t k,int32_t delta){encs[panel.enc[EN_K1+k]]=delta;frame();}
static void notes(const uint8_t *want){assert(kb_n[7]==3);assert(!memcmp(kb_nt[7],want,3));}
int main(int argc,char **argv)
{
 outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
 song.sel=0;song.seq_mode=0;song.octave=0;go_home();open_family(FAM_SCL);
 for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_VOICING)ui.page=p;
 ui.home=0;page_entered();frame();assert(cur_page()->scope==SC_VOICING);
 assert(!strcmp(ui.big_l[0],"INVERT")&&!strcmp(ui.big_l[1],"SPREAD"));
 uint8_t lock=ui.lock_par;
 turn(0,100);assert(TSEL->chord_inv==3&&ui.lock_par==lock);turn(0,-100);assert(TSEL->chord_inv==-3);
 turn(1,100);assert(TSEL->chord_spread==2);turn(1,-100);assert(TSEL->chord_spread==0);
 TSEL->chord_inv=0;TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_CHORD]=CH_MAJOR;TSEL->p[P_QUANT]=3;TSEL->p[P_AMODE]=0;
 fm1_in.notes=1u<<7;frame();notes((uint8_t[]){60,64,67});
 turn(0,1);turn(1,1);notes((uint8_t[]){60,64,67}); /* no reattack or pitch change while held */
 fm1_in.notes=0;frame();fm1_in.notes=1u<<7;frame();notes((uint8_t[]){64,72,79});
 fm1_in.notes=0;frame();assert(!chord_gates());TSEL->p[P_AHOLD]=1;
 fm1_in.notes=1u<<7;frame();fm1_in.notes=0;frame();assert(chord_latch_n[0]==3);
 uint8_t held[4];memcpy(held,chord_latch_notes[0],3);
 turn(0,-1);turn(1,1);assert(chord_latch_n[0]==3&&!memcmp(held,chord_latch_notes[0],3));
 turn(2,1);assert(TSEL->p[P_VLEAD]==1);turn(3,3);assert(TSEL->p[P_STRUM]>0);
 apply_preset(1);assert(TSEL->chord_inv==0&&TSEL->chord_spread==2);
 track_select(1);assert(!TSEL->chord_inv&&!TSEL->chord_spread);
 track_select(0);assert(TSEL->chord_spread==2);ui.force=1;frame();ppm("chord-voicing");
 /* Explicit inversions must not accumulate through automatic voicing history. */
 TSEL->chord_inv=1;TSEL->chord_spread=0;TSEL->p[P_VLEAD]=1;
 uint8_t first[4],next[4];uint32_t count=chord_play_notes(TSEL,60,0,first);
 for(uint32_t i=0;i<20;i++){assert(chord_play_notes(TSEL,60,0,next)==count);assert(!memcmp(first,next,count));}
 track_select(TRK_DRUM);int16_t *v;for(uint32_t k=0;k<4;k++)assert(!page_desc(cur_page(),k,&v));
 track_select(0);project_new();assert(!TSEL->chord_inv&&!TSEL->chord_spread);frame();
 /* Physical step entry and the arp consume the same literal voiced pitches. */
 TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_CHORD]=CH_MAJOR;TSEL->p[P_QUANT]=3;TSEL->p[P_TRANS]=0;
 TSEL->chord_inv=1;TSEL->chord_spread=1;song.octave=0;song.seq_mode=1;
 open_family(FAM_SEQ);ui.cursor=0;fm1_in.notes=1u<<7;frame();
 if(TSEL->step[0].n!=3||memcmp(TSEL->step[0].note,(uint8_t[]){64,72,79},3))
  fprintf(stderr,"entry: oct=%d trn=%d vl=%d inv=%d spr=%d page=%s mode=%u cursor=%u notes=%u [%u,%u,%u] key=%u [%u,%u,%u] panic=%u\n",song.octave,TSEL->p[P_TRANS],TSEL->p[P_VLEAD],TSEL->chord_inv,TSEL->chord_spread,cur_page()->title,song.seq_mode,ui.cursor,TSEL->step[0].n,TSEL->step[0].note[0],TSEL->step[0].note[1],TSEL->step[0].note[2],kb_n[7],kb_nt[7][0],kb_nt[7][1],kb_nt[7][2],panic_req);
 assert(TSEL->step[0].n==3&&!memcmp(TSEL->step[0].note,(uint8_t[]){64,72,79},3));
 fm1_in.notes=0;frame();song.seq_mode=0;go_home();TSEL->p[P_AMODE]=ARP_UP;
 fm1_in.notes=1u<<7;frame();assert(TSEL->nheld==3&&!memcmp(TSEL->held,(uint8_t[]){64,72,79},3));
 fm1_in.notes=0;frame();assert(!TSEL->nheld&&!TSEL->arp_n);
 puts("voicing UI: bounded controls, preset/track isolation, held/latch preservation and new-project reset PASS");
 return 0;
}
