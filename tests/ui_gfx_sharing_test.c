/* SPDX-License-Identifier: GPL-3.0-only */
/* Pixel/return-position oracle across palettes and clipped canvas passes. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint16_t expected[CV_MAX];
static uint32_t seed=7123;
static uint32_t next(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 panel=PANEL_DEFAULT;layers_init();host_tracks_init();
 int32_t (*f[])(int32_t,int32_t,const char *)={cv_text_c_gray,cv_text_c_dim,cv_text_c_hi,cv_text_c_white,cv_text_c_amb,cv_text_te_g4,cv_text_te_g3};
 for(unsigned p=0;p<NPALETTES;p++){
  palette_set(p);uint16_t colors[]={C_GRAY,C_DIM,C_HI,C_WHITE,C_AMB,TE_G4,TE_G3};
  for(unsigned i=0;i<1400;i++){
   unsigned h=1+next()%124,c=next()%7;int x=(int)(next()%300)-30,y=(int)(next()%180)-30,oy=(int)(next()%241)-120;
   const char *s=i&1?"NEON 123 +- / \xDC":"LONG CLIPPED LABEL";
   cv_begin(240,h,C_BLACK);cv_oy=oy;int a=cv_text(x,y,&FONT_S,s,colors[c]);memcpy(expected,cv_px,240*h*2);
   cv_band(h);cv_oy=oy;int b=f[c](x,y,s);assert(a==b&&!memcmp(expected,cv_px,240*h*2));
   cv_band(h);cv_oy=oy;b=cv_text_s(x,y,s,colors[c]);assert(a==b&&!memcmp(expected,cv_px,240*h*2));
  }
 }
 cv_oy=0;
 /* Exact deferred display boundary, followed by immediate encoder ownership. */
 go_home();fm1_in.buttons=fm1_in.notes=0;ui_input();
 fm1_ms=10000;fm1_in.buttons=BT(B_FX);edges_btn=BT(B_FX);ui_input();edges_btn=0;assert(ui.layer==LY_PLAY);
 fm1_ms=10349;ui_input();assert(ui.layer==LY_PLAY);
 fm1_ms=10350;ui_input();assert(ui.layer==LY_FX);
 fm1_in.buttons=0;ui_input();go_home();
 fm1_ms=11000;fm1_in.buttons=BT(B_FX);edges_btn=BT(B_FX);ui_input();edges_btn=0;
 encs[panel.enc[EN_K1]]=1;ui_input();assert(ui.layer==LY_FX);
 puts("UI sharing: clipped font/palette oracle and 349/350 ms/immediate ownership PASS");return 0;
}
