/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static const struct {unsigned e,p;const char *name;int trim;} sounds[]={
 {0,23,"SYN VIOLIN",-6},{1,26,"REV METAL",9},{1,27,"REV HARP",12},{5,7,"REV CHOIR",0},
 {3,27,"GB RAZOR",7},{3,28,"NES SPARK",16},{1,28,"MD CRYSTAL",12},{1,29,"MD PICK",21},
 {0,24,"NIGHT PAD",-2},{2,13,"SLASH STR",-9},{5,8,"HEX CHOIR",-20},{1,30,"DREAD BELL",0},
 {2,14,"ORBIT PAD",-6},
 {1,31,"STAR KEYS",5},
 {5,9,"LUNAR VOX",-7},
 {10,28,"ISLAND PAN",-5},
 {10,29,"DESERT OUD",3},
 {7,8,"DUSK ORGAN",0},
 {3,29,"PIXEL BASS",23},
 {3,30,"SAVE BELL",7},
 {2,15,"CRT LEAD",-7},
 {8,8,"GLASS RAIN",23},
 {7,9,"TOY ORGAN",23},
 {5,10,"ROBOT TALK",-5}
};
int main(void) {
 for(unsigned k=0;k<NELEM(sounds);k++) {
  const engine_t *e=ENGINES[sounds[k].e];const preset_t *p=&e->presets[sounds[k].p];
  assert(!strcmp(p->name,sounds[k].name));assert(preset_trim_value(sounds[k].e,sounds[k].p)==sounds[k].trim);
  for(unsigned old=0;old<sounds[k].p;old++)assert(memcmp((const char *)p+sizeof p->name,(const char *)&e->presets[old]+sizeof p->name,32));
  for(unsigned note=48;note<=84;note+=12)for(unsigned vel=40;vel<=120;vel+=40) {
   host_tracks_init();host_preset(&trk[0],sounds[k].e,sounds[k].p);track_t *t=&trk[0];
   t->p[P_DIST]=t->p[P_CHOR]=t->p[P_DLY]=t->p[P_REV]=0;trk_note_on(t,note,vel);
   uint64_t energy=0,tail=0;
   unsigned seconds=k>=12?5u:3u; /* The vacation pads deliberately have longer releases. */
   for(unsigned b=0;b<seconds*FS/CTL;b++) {
    if(b==2u*FS/CTL)trk_note_off(t,note);int32_t out[CTL];track_render(t,out,CTL);
    for(unsigned i=0;i<CTL;i++){assert(abs(out[i])<1000000);energy+=(uint32_t)abs(out[i]);if(b>=(4u*seconds-1u)*FS/(4u*CTL))tail+=(uint32_t)abs(out[i]);}
   }
   if(energy<=10000||tail>=10000)fprintf(stderr,"%s note %u velocity %u energy %llu tail %llu\n",sounds[k].name,note,vel,(unsigned long long)energy,(unsigned long long)tail);
   assert(energy>10000&&tail<10000);for(unsigned v=0;v<NVOICE;v++)assert(!t->v[v].active);
  }
 }
 puts("Expanded sounds: all added identities/trims, distinct recipes, pitch/velocity bounds and clean release PASS");return 0;
}
