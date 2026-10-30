/* SPDX-License-Identifier: GPL-3.0-only */
/* Appended console/old-digital presets: ranges, distinct recipes and audible output. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static const struct {uint8_t engine,preset;const char *name;} ADDED[]={
 {3,20,"NES REED"},{3,21,"NES BUBBLE"},{3,22,"GB MALLET"},{3,23,"GB VOX"},
 {1,24,"MD BASS"},{1,25,"MD BRASS"},{3,24,"WORN EP"},{3,25,"DUST ORGAN"}};
int main(int argc,char **argv)
{
 assert(argc==3);
 FILE *w=fopen(argv[1],"wb"),*rep=fopen(argv[2],"w");assert(w&&rep);wav_hdr(w,0);
 uint32_t total=0;int32_t out[2*CTL];
 for(uint32_t a=0;a<NELEM(ADDED);a++) {
  const engine_t *e=ENGINES[ADDED[a].engine];const preset_t *p=&e->presets[ADDED[a].preset];
  assert(!strcmp(p->name,ADDED[a].name));
  for(uint32_t k=0;k<8;k++)assert(p->e[k]>=e->edit[k].min&&p->e[k]<=e->edit[k].max);
  for(uint32_t old=0;old<(ADDED[a].engine==3?20u:24u);old++)
   assert(memcmp((const uint8_t*)p+sizeof p->name,(const uint8_t*)&e->presets[old]+sizeof p->name,32));
  host_tracks_init();host_preset(&trk[0],ADDED[a].engine,ADDED[a].preset);
  /* Run out shared bus tails before each isolated example. */
  for(uint32_t b=0;b<FS/CTL;b++)mix_block(out,CTL);
  uint64_t energy=0;int32_t peak=0;uint32_t changes=0;int32_t previous=0;
  static const uint8_t phrase[]={60,63,67,70};
  uint32_t section=(FS*3/CTL),note=0;
  fprintf(rep,"%u %.3f %s\n",a,(double)total/FS,p->name);
  for(uint32_t b=0;b<section;b++) {
   uint32_t q=b/(FS/2/CTL);
   if(b%(FS/2/CTL)==0 && q<4u) {
    if(q)trk_note_off(&trk[0],note);
    note=phrase[q];trk_note_on(&trk[0],note,100);
   }
   if(b==4*(FS/2/CTL))trk_note_off(&trk[0],note);
   mix_block(out,CTL);
   for(uint32_t i=0;i<CTL;i++) {
    int32_t x=(out[2*i]+out[2*i+1])/2,absx=x<0?-x:x;
    if(absx>peak)peak=absx;energy+=(int64_t)x*x;changes+=x!=previous;previous=x;
    wav_put(w,out[2*i],out[2*i+1]);total++;
   }
  }
  assert(peak>64&&peak<=32768&&energy>1000000&&changes>1000);
  printf("%s: peak %d RMS %.2f changes %u PASS\n",p->name,peak,sqrt((double)energy/(section*CTL)),changes);
 }
 fseek(w,0,SEEK_SET);wav_hdr(w,total);assert(!fclose(w)&&!fclose(rep));
 puts("Eight appended presets: bounds, distinct recipes, audible rendering and demo WAV PASS");return 0;
}
