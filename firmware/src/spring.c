/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * Dispersive allpass topology adapted from illuminus5/skiff, commit
 * eda8de411e5b1314e6f5d4a7170ab9cd886f5845, firmware/src/fx.c.
 * MerthSloop: two unequal spring paths, independent pickups, existing storage. */
#define SP_MASK 4095u
#define SP_N 8u
static struct {
    uint32_t w,phase;
    int32_t size,lp[2],hp[2],err[2],pick[2],gain[2];
} spring;
static struct { uint8_t cur; int32_t gain; } reverb_model = {0,32768};
_Static_assert(sizeof rev_line/sizeof *rev_line>=8192,"two spring paths fit existing reverb storage");
static void spring_begin(void)
{
    int32_t len=(int32_t)((uint32_t)clamp(song.g[G_RTIME],20,80)*FS/1000u)<<8;
    if(!spring.size)spring.size=len;
    if(!rev_frz)spring.size+=clamp(len-spring.size,-256,256);
    uint32_t rt=400u+(uint32_t)song.g[G_RSIZE]*40u;
    spring.phase+=2u*LFO_INC[24];
    for(uint32_t c=0;c<2;c++) {
        uint32_t length=((uint32_t)spring.size>>8);
        if(c)length-=length>>4; /* two slightly different physical spring lengths */
        uint32_t loss=653116u*length/(FS*rt/1000u);
        spring.gain[c]=(int32_t)(px_exp2(-(int32_t)loss)>>1);
        spring.pick[c]=(int32_t)(length<<7)+((osc_sine(spring.phase+c*0x40000000u)*3)>>8);
    }
}
static void spring_sample(int32_t input,int32_t *left,int32_t *right)
{
    uint32_t wp=spring.w,j=(wp&3u)*(SP_N+1u);
    int32_t out[2];
    for(uint32_t c=0;c<2;c++) {
        int16_t *line=rev_line+c*(SP_MASK+1u);
        uint32_t L=(uint32_t)spring.size>>8;
        if(c)L-=L>>4;
        int32_t pick=spring.pick[c];
        int32_t r=line[(wp-L)&SP_MASK],x=clamp(mulq16(input,3200u<<1),-131072,131072);
        int32_t t0=line[(wp-((uint32_t)pick>>8))&SP_MASK];
        int32_t t1=line[(wp-((uint32_t)pick>>8)-1u)&SP_MASK];
        int32_t second=line[(wp-((L*3u)>>2))&SP_MASK];
        if(rev_frz) {
            /* No modulation/damping in the lossless held feedback path. */
            x+=r;
        } else {
            spring.lp[c]+=mulq15(r-spring.lp[c],26000-song.g[G_RDAMP]*160);
            int32_t o=spring.lp[c]*spring.gain[c];
            x+=(o+((o>>31)&32767))>>15;
            o=x-spring.hp[c]+spring.err[c];spring.err[c]=o&63;spring.hp[c]+=o>>6;x-=spring.hp[c];
            x=clamp(x,-131072,131072);
            int32_t *ap=rev_storage.spring[c],p=ap[j];ap[j]=x;
            for(uint32_t k=1;k<=SP_N;k++) {
                int32_t v=(x-ap[j+k])*(c?2703:2867),prev=ap[j+k];
                x=clamp(((v+((v>>31)&4095))>>12)+p,-131072,131072);p=prev;ap[j+k]=x;
            }
        }
        line[wp&SP_MASK]=(int16_t)clamp(x,-32768,32767);
        int32_t first=t0+(((t1-t0)*(pick&255))>>8);
        out[c]=first*4+second*2;
    }
    spring.w=wp+1u;*left=out[0];*right=out[1];
}
static uint32_t reverb_begin(void)
{
    uint32_t want=(uint32_t)clamp(song.g[G_RMODEL],0,5);
    if(!reverb_model.gain && want!=reverb_model.cur && !rev_frz) {
        memset(rev_line,0,sizeof rev_line);memset(&rev_storage,0,sizeof rev_storage);
        memset(fx.line_i,0,sizeof fx.line_i);memset(fx.ap_i,0,sizeof fx.ap_i);
        memset(fx.line_lp,0,sizeof fx.line_lp);memset(&spring,0,sizeof spring);
        reverb_model.cur=(uint8_t)want;
    }
    if(reverb_model.cur==1)spring_begin();
    return want;
}
