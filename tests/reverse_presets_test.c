/* SPDX-License-Identifier: GPL-3.0-only */
/* Reverse-style envelopes must swell, fall silent while held, and release cleanly. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static const struct {unsigned e,p;const char *name;} sounds[]={
 {0,22,"REV STR"},{3,26,"REV HAZE"},{2,12,"REV REED"}
};
int main(int argc,char **argv) {
 FILE *wav=argc>1?fopen(argv[1],"wb"):0;uint32_t frames=0;
 if(wav)wav_hdr(wav,0);
 for(unsigned k=0;k<NELEM(sounds);k++)for(unsigned note=48;note<=84;note+=12)for(unsigned velocity=40;velocity<=120;velocity+=40) {
  host_tracks_init();host_preset(&trk[0],sounds[k].e,sounds[k].p);track_t *t=&trk[0];
  assert(!strcmp(ENGINES[sounds[k].e]->presets[sounds[k].p].name,sounds[k].name));
  t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;
  uint32_t attack=(TIME_MS_X10[t->p[P_ATK]]*FS/10000u)/CTL;
  uint64_t early=0,crest=0,tail=0;unsigned width=attack/8u;assert(width>0);
  trk_note_on(t,note,velocity);
  for(unsigned b=0;b<3u*FS/CTL;b++) {
   int32_t out[CTL];if(b==2u*FS/CTL)trk_note_off(t,note);
   track_render(t,out,CTL);
   for(unsigned i=0;i<CTL;i++) {
    assert(abs(out[i])<1000000);uint32_t a=(uint32_t)abs(out[i]);
    if(b<width)early+=a;
    if(b>=attack-width&&b<attack)crest+=a;
    if(b>=attack+FS/(4u*CTL)&&b<2u*FS/CTL)tail+=a;
    if(wav&&note==60&&velocity==80) {wav_put(wav,out[i]/2,out[i]/2);frames++;}
   }
  }
  assert(crest>early*2u && crest>10000u);assert(tail<crest/100u);
  for(unsigned v=0;v<NVOICE;v++)assert(!t->v[v].active);
 }
 if(wav){fseek(wav,0,SEEK_SET);wav_hdr(wav,frames);fclose(wav);}
 puts("Reverse presets: range/velocity swell, crest, held silence and note-off cleanup PASS");return 0;
}
