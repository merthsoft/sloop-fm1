/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * SPRING adapted from illuminus5/skiff, commit
 * eda8de411e5b1314e6f5d4a7170ab9cd886f5845, firmware/src/fx.c.
 * Narrow adapter: existing stereo buses, shared storage, model ramps and FREEZE.
 * Ten stretched allpasses produce dispersive spring chirps; no extra delay line. */
#define SP_MASK 4095u
#define SP_N 10u
#define SP_A 2867
static struct { uint32_t w,phase; int32_t size,lp,hp,err,pick,gain; } spring;
static struct { uint8_t cur; int32_t gain; } reverb_model = {0,32768};
_Static_assert(sizeof rev_line/sizeof *rev_line>=4096,"spring line fits existing reverb storage");
static void spring_begin(void)
{
    int32_t len=(int32_t)((uint32_t)clamp(song.g[G_RTIME],20,80)*FS/1000u)<<8;
    if(!spring.size)spring.size=len;
    if(!rev_frz)spring.size+=clamp(len-spring.size,-256,256);
    /* Normalize feedback to a nominal RT60, so TIME does not also set decay.
     * px_exp2 already serves the physical engines; once per control block. */
    uint32_t rt=400u+(uint32_t)song.g[G_RSIZE]*40u;
    uint32_t loss=653116u*((uint32_t)spring.size>>8)/(FS*rt/1000u);
    spring.gain=(int32_t)(px_exp2(-(int32_t)loss)>>1);
    spring.phase+=2u*LFO_INC[24];
    spring.pick=(spring.size>>1)+((osc_sine(spring.phase)*3)>>8);
}
static void spring_sample(int32_t input,int32_t *left,int32_t *right)
{
    uint32_t wp=spring.w,L=(uint32_t)spring.size>>8,j=(wp&3u)*(SP_N+1u);
    int32_t r=rev_line[(wp-L)&SP_MASK],x=clamp(mulq16(input,2580u<<1),-131072,131072);
    int32_t t0=rev_line[(wp-((uint32_t)spring.pick>>8))&SP_MASK];
    int32_t t1=rev_line[(wp-((uint32_t)spring.pick>>8)-1u)&SP_MASK];
    int32_t second=rev_line[(wp-((L*3u)>>2))&SP_MASK];
    if(rev_frz) {
        /* Lossless fixed delay while held: no damping, dispersion or size modulation
         * in the feedback path. Existing captured send policy bounds new input. */
        x+=r;
    } else {
        int32_t g=spring.gain;
        spring.lp+=mulq15(r-spring.lp,26000-song.g[G_RDAMP]*160);
        int32_t o=spring.lp*g;
        x+=(o+((o>>31)&32767))>>15;
        o=x-spring.hp+spring.err; spring.err=o&63; spring.hp+=o>>6; x-=spring.hp;
        /* Four times line full scale keeps all allpass products inside int32
         * even on an overloaded send. Normal tails stay below this guard. */
        x=clamp(x,-131072,131072);
        int32_t *ap=rev_storage.spring,p=ap[j];ap[j]=x;
        for(uint32_t k=1;k<=SP_N;k++) {
            int32_t v=(x-ap[j+k])*SP_A,o=ap[j+k];
            x=clamp(((v+((v>>31)&4095))>>12)+p,-131072,131072); p=o;ap[j+k]=x;
        }
    }
    rev_line[wp&SP_MASK]=(int16_t)clamp(x,-32768,32767);spring.w=wp+1u;
    int32_t first=t0+(((t1-t0)*(spring.pick&255))>>8);
    *left=first*4+second*2; *right=second*4+first*2;
}
static void reverb_begin(void)
{
    uint32_t want=(uint32_t)song.g[G_RMODEL]&1u;
    if(!reverb_model.gain && want!=reverb_model.cur && !rev_frz) {
        memset(rev_line,0,sizeof rev_line);memset(&rev_storage,0,sizeof rev_storage);
        memset(fx.line_i,0,sizeof fx.line_i);memset(fx.ap_i,0,sizeof fx.ap_i);
        memset(fx.line_lp,0,sizeof fx.line_lp);memset(&spring,0,sizeof spring);
        reverb_model.cur=(uint8_t)want;
    }
    if(reverb_model.cur)spring_begin();
}
