/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint32_t digest=2166136261u;
static void add(uint32_t v){digest=(digest^v)*16777619u;}
int main(void){
 panel=PANEL_DEFAULT;layers_init();host_tracks_init();
 const uint32_t items[]={MI_RATE,MI_BUDGET,MI_LIGHTS,MI_KEYS,MI_SCALELEVEL};
 const int32_t turns[]={-128,-7,-1,0,1,7,128};
 for(unsigned i=0;i<5;i++)for(unsigned value=0;value<256;value++)for(unsigned turn=0;turn<7;turn++){
  usb_rate=voice_expand=lights_keys=scale_light_level=lights_lvl=(uint8_t)value;
  mi_set(items[i],turns[turn]);
  add(usb_rate);add(voice_expand);add(lights_keys);add(scale_light_level);add(lights_lvl);
 }
 for(unsigned k=0;k<4;k++)for(int value=-100;value<=200;value++)for(unsigned turn=0;turn<7;turn++){
  song.g[G_FILT]=song.g[G_DUST]=song.g[G_DUCK]=TSEL->p[P_TFLT]=(int16_t)value;
  enc_drop();encs[EN_K1+k]=turns[turn];knob_accel_off=1;
  layer_knobs(LY_FX);
  add((uint16_t)song.g[G_FILT]);add((uint16_t)song.g[G_DUST]);add((uint16_t)song.g[G_DUCK]);add((uint16_t)TSEL->p[P_TFLT]);
 }
 assert(digest==0x394d41c8u); /* Frozen original menu/held-FX state oracle. */
 printf("state oracle %08x PASS\n",digest);
 return 0;
}
