/* SPDX-License-Identifier: GPL-3.0-only */
/* Canonical metadata and all displayed values: checked against frozen .103 output. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static FILE *out;
static void emit(const param_desc_t *d) {
 fprintf(out,"%u,%d,%u,%d,%s,%s",param_fmt(d),d->min,d->max,d->def,d->label,param_unit(d));
 if(d->fmt==F_ENUM&&d->names)for(int v=d->min;v<=d->max;v++)fprintf(out,",%s",d->names[v]);
 fputc('\n',out);
 for(int v=d->min;v<=d->max;v++) {char s[32];const char *u;param_format(d,v,s,&u);fprintf(out,"%d:%s:%s\n",v,s,u);}
}
int main(int argc,char **argv) {
 assert(argc==2);out=fopen(argv[1],"wb");assert(out);
 host_tracks_init();
 for(uint32_t i=0;i<P_COUNT;i++)emit(&TP[i]);
 for(uint32_t i=0;i<G_COUNT;i++)emit(&GP[i]);
 for(uint32_t e=0;e<NENGINES;e++)for(uint32_t p=0;p<ENGINES[e]->npresets;p++) {
  host_preset(&trk[0],e,p);
  for(uint32_t i=0;i<8;i++)emit(track_desc(&trk[0],P_E0+i));
 }
 for(uint32_t type=0;type<NELEM(N_ITYPE);type++)for(uint32_t i=0;i<3;i++)emit(ins_desc(type,i));
 for(uint32_t i=0;i<NELEM(ESYNC_DESC);i++)emit(&ESYNC_DESC[i]);
 for(uint32_t i=0;i<NELEM(DRIFT_DESC);i++)emit(&DRIFT_DESC[i]);
 for(uint32_t i=0;i<NELEM(EVOLVE_DESC);i++)emit(&EVOLVE_DESC[i]);
 for(uint32_t i=0;i<NELEM(SUSTAIN_DESC);i++)emit(&SUSTAIN_DESC[i]);
 for(uint32_t i=0;i<NELEM(SNAPSHOT_DESC);i++)emit(&SNAPSHOT_DESC[i]);
 emit(&DUCK_TARGET);emit(&VOICE_INVERT);emit(&VOICE_SPREAD);emit(&REC_SNAP_DESC);
 assert(!fclose(out));return 0;
}
