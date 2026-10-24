/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#define DSP_INLINE static inline __attribute__((always_inline))
#define main hostsim_main
#include "hostsim.c"
#undef main
static int32_t reference(int32_t v,int32_t lo,int32_t hi){return v<lo?lo:v>hi?hi:v;}
static uint32_t clamp_seed=0x28058;
static uint32_t clamp_rand32(void){clamp_seed^=clamp_seed<<13;clamp_seed^=clamp_seed>>17;clamp_seed^=clamp_seed<<5;return clamp_seed;}
int main(void){
 int32_t e[]={INT32_MIN,INT32_MIN+1,-131072,-65537,-65536,-32768,-1,0,1,32767,65535,65536,131072,INT32_MAX-1,INT32_MAX};
 unsigned count=0;
 for(unsigned i=0;i<15;i++)for(unsigned j=0;j<15;j++)for(unsigned k=0;k<15;k++){int32_t v=e[i],lo=e[j],hi=e[k];assert(clamp(v,lo,hi)==reference(v,lo,hi));count++;}
 for(unsigned i=0;i<1000000;i++){int32_t v=clamp_rand32(),lo=clamp_rand32(),hi=clamp_rand32();assert(clamp(v,lo,hi)==reference(v,lo,hi));assert(clamp(v,-131072,131072)==reference(v,-131072,131072));count+=2;}
 int n=0;assert(clamp(n++,-1,1)==0 && n==1);assert(clamp(0,0,UINT32_MAX)==-1);assert(clamp(0,10,-10)==10);
 int lo=1,hi=4;assert(clamp(2,lo++,hi++)==2 && lo==2 && hi==5);
 printf("portable clamp: %u comparisons; single evaluation and unsigned-to-int32 bounds PASS\n",count);
 return 0;
}
