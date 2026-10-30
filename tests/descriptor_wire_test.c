/* SPDX-License-Identifier: GPL-3.0-only */
#define SNAPSHOT_PERSISTENCE_FIXTURE
#include "headroom_commit_test.c"
#undef main
int main(int argc,char **argv) {
 assert(argc==2);setup();FILE *f=fopen(argv[1],"wb");assert(f);
 for(unsigned scope=0;scope<2;scope++)for(unsigned e=0;e<(scope?1u:NENGINES);e++)
 for(unsigned p=0;p<(scope?1u:ENGINES[e]->npresets);p++) {
  if(!scope)host_preset(&trk[0],e,p);song.sel=0;
  for(unsigned id=0;id<(scope?G_COUNT:P_COUNT);id++) {
   uint8_t a[2]={(uint8_t)scope,(uint8_t)id};ed_n=0;request(ED_DESC,a,2);
   assert(ed_n>7 && ed_n<sizeof ed_out && ed_out[4]==ED_DESC);
   assert(ed_out[7]<=F_FILT); /* Internal unit formats must never reach the wire. */
   assert(fwrite(&ed_n,sizeof ed_n,1,f)==1 && fwrite(ed_out,ed_n,1,f)==1);
  }
 }
 assert(!fclose(f));return 0;
}
