/* SPDX-License-Identifier: GPL-3.0-only */
/* Compile raw and boot-restored, then compare canonical factory bytes and names. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(int argc,char **argv){
 assert(argc==2);host_tracks_init();
#if FELUCCA_BOOT_PRESETS
 uint8_t scratch[3818];memset(scratch,0xa5,sizeof scratch);
 assert(!boot_presets_init(scratch+1,0));
 assert(scratch[0]==0xa5&&scratch[sizeof scratch-1]==0xa5);
 assert(boot_presets_init(scratch+1,sizeof scratch-2));
 assert(scratch[0]==0xa5&&scratch[sizeof scratch-1]==0xa5);
#endif
 FILE *out=fopen(argv[1],"wb");assert(out);unsigned count=0;
 for(unsigned k=0;k<NENGINES;k++)for(unsigned i=0;i<ENGINES[k]->npresets;i++){
  const preset_t *p=&ENGINES[k]->presets[i];
  assert(fwrite(p->name,strlen(p->name)+1,1,out)==1);
  assert(fwrite((const uint8_t *)p+sizeof p->name,32,1,out)==1);count++;
 }
 assert(!fclose(out));printf("factory presets: %u complete records, names, startup scratch bounds and repeated restore PASS\n",count);return 0;
}
