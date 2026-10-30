/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static uint16_t expected_pixels[CV_MAX];
static uint32_t rseed=91671u;
static uint32_t rnd(void) { rseed=rseed*1664525u+1013904223u;return rseed; }
static int32_t ref_width(const felucca_font_t *f,const char *s) {int32_t w=0;for(;*s;s++)w+=font_advance(f,glyph(f,(uint8_t)*s));return w;}
static void ref_trim(char *s,const felucca_font_t *f,int32_t w) {while(s[0]&&ref_width(f,s)>w)s[str_len(s)-1u]=0;}
static void ref_card(int32_t x,int32_t y,const char *label,const char *value,int32_t ratio,uint16_t color,int hot)
{
 char l[8],v[12];const felucca_font_t *f=ref_width(&FONT_L,value)<=50?&FONT_L:&FONT_S;
 str_cpy(l,label,sizeof l);str_cpy(v,value,sizeof v);
 while(ref_width(&FONT_S,l)>48)l[str_len(l)-1u]=0;
 while(ref_width(f,v)>48)v[str_len(v)-1u]=0;
 cv_rect(x,y,56,44,hot?color:TE_G2);cv_rect(x+1,y+1,54,42,TE_G1);
 if(!label[0]){cv_text(x+20,y+14,&FONT_S,"--",TE_G2);return;}
 cv_text(x+4,y,&FONT_S,l,TE_G4);cv_text(x+(56-ref_width(f,v))/2,y+(f==&FONT_L?10:19),f,v,color);
 if(ratio>=0)for(int32_t i=0;i<10;i++)cv_rect(x+4+i*5,y+40,3,2,i*100<ratio?color:TE_G2);
}
static void ref_digit(int32_t x,uint32_t d)
{
 const uint8_t SEG[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};uint32_t m=SEG[d%10u];
 int32_t y=4,w=32,h=56,t=7,hh=h/2;uint16_t c=C_WHITE;
 if(m&1)cv_rect(x+t,y,w-2*t,t,c);if(m&2)cv_rect(x+w-t,y+t,t,hh-t,c);if(m&4)cv_rect(x+w-t,y+hh,t,hh-t,c);
 if(m&8)cv_rect(x+t,y+h-t,w-2*t,t,c);if(m&16)cv_rect(x,y+hh,t,hh-t,c);if(m&32)cv_rect(x,y+t,t,hh-t,c);if(m&64)cv_rect(x+t,y+hh-t/2,w-2*t,t,c);
}
int main(void)
{
 panel=PANEL_DEFAULT;layers_init();host_tracks_init();
 uint8_t adv[224];for(unsigned i=0;i<224;i++)adv[i]=(uint8_t)(1+i%19);felucca_font_t prop=FONT_S;prop.adv=adv;
 unsigned cases=0;
 for(unsigned p=0;p<NPALETTES;p++){palette_set(p);
  for(unsigned i=0;i<2000;i++){
   char str[72],a[72],b[72];unsigned n=rnd()%71;for(unsigned j=0;j<n;j++)str[j]=(char)(1+rnd()%255);str[n]=0;
   assert(text_w_s(str)==ref_width(&FONT_S,str));assert(text_w_l(str)==ref_width(&FONT_L,str));
   const felucca_font_t *f=i%3==0?&prop:i%3==1?&FONT_S:&FONT_L;int32_t maxw=(int32_t)(rnd()%130)-1;
   str_cpy(a,str,sizeof a);str_cpy(b,str,sizeof b);ref_trim(a,f,maxw);text_trim(b,f,maxw);assert(!strcmp(a,b));
   int32_t x=(int32_t)(rnd()%320)-40,y=(int32_t)(rnd()%180)-30;unsigned h=1+rnd()%124;int32_t oy=(int32_t)(rnd()%80)-40;
   int32_t ratio=(int32_t)(rnd()%1300)-100;uint16_t color=TE_COL[i%4];int hot=i&1;
   cv_band(h);cv_oy=oy;ref_card(x,y,str,str,ratio,color,hot);memcpy(expected_pixels,cv_px,240*h*2);
   cv_band(h);cv_oy=oy;ui_card(x,y,str,str,ratio,color,hot);assert(!memcmp(expected_pixels,cv_px,240*h*2));
   cv_band(h);cv_oy=oy;ref_digit(x,i%20);memcpy(expected_pixels,cv_px,240*h*2);
   cv_band(h);cv_oy=oy;te_rec_digit(x,i%20);assert(!memcmp(expected_pixels,cv_px,240*h*2));
   cases++;
  }
 }
 cv_oy=0;printf("drawing oracle: %u cases, widths/proportional fallback/trim/cards/REC digit exact PASS\n",cases);return 0;
}
