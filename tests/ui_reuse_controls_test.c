/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(void) {
 assert(TP[P_ARATE].names==N_SDIV && GP[G_DREC].names==N_SDIV);
 assert(TP[P_ARATE].max==5 && GP[G_DREC].max==5 && TP[P_SDIV].max==8);
 panel=PANEL_DEFAULT;layers_init();host_tracks_init();unsigned cases=0;
 for(unsigned family=0;family<2;family++)for(unsigned id=0;id<(family?G_COUNT:P_COUNT);id++) {
  const param_desc_t *d=family?&GP[id]:&TP[id];
  for(unsigned slot=0;slot<4;slot++)for(int step=-20;step<=20;step++)for(unsigned off=0;off<2;off++)for(unsigned state=0;state<5;state++) {
   knob_accel_off=off;fm1_ms=1000;uint32_t role=EN_K1+slot;
   uint32_t st=state==0?0:(980u | ((state&1)<<24) | ((state==4?127u:10u)<<25));
   int v=(d->min+d->max)/2;
   ui.enc_t[role]=st;
   int old=clamp(v+accel(role,step,d->names?0:d->max-d->min),d->min,d->max);uint32_t oldst=ui.enc_t[role];
   ui.enc_t[role]=st;
   assert(param_turn(d,v,slot,step)==old);assert(ui.enc_t[role]==oldst);cases++;
  }
 }
 printf("parameter turns: %u exact values and acceleration states PASS\n",cases);return 0;
}
