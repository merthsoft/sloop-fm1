/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/src/wet_color.c"

static void settle(wet_color_t *s, unsigned mode)
{
    for (int i=0;i<600;i++) { int32_t f[2]={0,0}; wet_color_frame(s,f,2,mode,127); }
}
int main(void)
{
    wet_color_t s={0};
    int32_t extremes[]={INT_MIN,INT_MAX,-32768,32768,0,1234567};
    for(unsigned i=0;i<sizeof extremes/sizeof *extremes;i++) {
        int32_t f[2]={extremes[i],extremes[i]};
        wet_color_frame(&s,f,2,0,127); assert(f[0]==extremes[i]&&f[1]==f[0]);
    }
    for(unsigned mode=1;mode<=2;mode++) {
        memset(&s,0,sizeof s); settle(&s,mode);
        for(int i=0;i<10000;i++) {
            int32_t f[2]={i&1?INT_MAX:INT_MIN,0};
            wet_color_frame(&s,f,2,mode,127);
            assert(f[0]>=-32768&&f[0]<=32768&&f[1]==0);
        }
        settle(&s,mode);
        int32_t zero[2]={0,0}; wet_color_frame(&s,zero,2,mode,127);
        assert(zero[0]==0&&zero[1]==0);
        for(int i=0;i<600;i++) {int32_t f[2]={1234,-5678};wet_color_frame(&s,f,2,0,0);}
        int32_t f[2]={INT_MIN,INT_MAX};wet_color_frame(&s,f,2,0,0);
        assert(f[0]==INT_MIN&&f[1]==INT_MAX);
    }
    double power[2]={0,0};
    for(int k=0;k<2;k++) {
        memset(&s,0,sizeof s); settle(&s,1);
        for(int i=0;i<44100;i++) {
            int32_t f[2]={(int32_t)(2000*sin(6.283185307179586*(k?10000:200)*i/44100)),0};
            wet_color_frame(&s,f,2,1,127);if(i>1000)power[k]+=(double)f[0]*f[0];
        }
    }
    assert(power[1]/power[0]<.03);
    printf("TAPE 10kHz/200Hz power: %.6f\n",power[1]/power[0]);
    memset(&s,0,sizeof s);settle(&s,2);
    int32_t prev=0; unsigned held=0; long long sum=0;
    for(int i=0;i<4096;i++) {
        int32_t f[2]={(i%256-128)*211, -(i%256-128)*211};
        wet_color_frame(&s,f,2,2,127);
        assert(f[0]%256==0&&f[0]==-f[1]);
        if(i&&f[0]==prev)held++;
        prev=f[0];sum+=f[0];
    }
    assert(held>=3072); (void)sum;
    /* Static transfer: symmetric DC, compression, exact quantized DC. */
    for(unsigned mode=1;mode<=2;mode++) {
        memset(&s,0,sizeof s);
        int32_t f[2];
        for(int i=0;i<2000;i++) {
            f[0]=24000;f[1]=-24000;wet_color_frame(&s,f,2,mode,127);
        }
        assert(f[0]==-f[1]);
        if(mode==1)assert(f[0]>14000&&f[0]<16000);
        else assert(f[0]==24064);
    }
    /* Fixed DC, mode/bypass changes: bounded step for constant input. */
    memset(&s,0,sizeof s);prev=12000;
    for(int i=0;i<10000;i++) {
        int32_t f[2]={12000,-12000};
        wet_color_frame(&s,f,2,(unsigned)(i/1000)%3,127);
        assert(abs(f[0]-prev)<100&&f[0]==-f[1]);prev=f[0];
        assert(s.tape+s.crush<=32768);
    }
    /* Independent instances, deterministic replay, and block splitting. */
    wet_color_t a={0},b={0},mono={0};
    for(int i=0;i<20000;i++) {
        int32_t x[2]={i*991%65000-32500,i*71%64000-32000},y[2]={x[0],x[1]},z=x[0];
        unsigned mode=(unsigned)(i/317)%3,amount=(unsigned)i%128;
        wet_color_frame(&a,x,2,mode,amount);wet_color_frame(&b,y,2,mode,amount);
        wet_color_frame(&mono,&z,1,mode,amount);
        assert(x[0]==y[0]&&x[1]==y[1]&&z==x[0]);
    }
    assert(memcmp(&a,&b,sizeof a)==0);
    memset(&s,0,sizeof s);
    for(int i=0;i<10000;i++) {
        int32_t f[2]={INT_MIN,INT_MAX};
        wet_color_frame(&s,f,2,(unsigned)(i/79)%4,999);
        assert(f[0]<=0&&f[1]>=0&&s.tape+s.crush<=32768);
    }
    wet_color_t saved=s;int32_t untouched[2]={123,-456};
    wet_color_frame(&s,untouched,0,1,127);
    wet_color_frame(&s,untouched,3,1,127);
    assert(memcmp(&s,&saved,sizeof s)==0&&untouched[0]==123&&untouched[1]==-456);
    printf("wet_color: all assertions passed; state %zu bytes\n",sizeof s);
    return 0;
}
