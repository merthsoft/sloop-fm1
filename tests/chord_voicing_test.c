/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/harmony_voicing.h"

static void expect(const uint8_t *input, const uint8_t *output, uint32_t n,
                   int32_t inversion, uint32_t spread)
{
    uint8_t actual[4];
    memcpy(actual,input,n);
    harmony_shape(actual,n,inversion,spread);
    if(memcmp(actual,output,n)) {
        fprintf(stderr,"inversion %d spread %u:",inversion,spread);
        for(uint32_t i=0;i<n;i++) fprintf(stderr," %u/%u",actual[i],output[i]);
        fputc('\n',stderr);
    }
    assert(!memcmp(actual,output,n));
}

static void invariant(const uint8_t *input, uint32_t n, int32_t inversion, uint32_t spread)
{
    uint8_t a[6],b[4];
    uint32_t before[12]={0},after[12]={0};
    memset(a,213,sizeof a);
    memcpy(a+1,input,n);memcpy(b,input,n);
    harmony_shape(a+1,n,inversion,spread);
    harmony_shape(b,n,inversion,spread);
    assert(a[0]==213);
    for(uint32_t i=n+1;i<sizeof a;i++) assert(a[i]==213);
    assert(!memcmp(a+1,b,n));
    for(uint32_t i=0;i<n;i++) {
        assert(a[i+1]<128);
        if(i) assert(a[i]<a[i+1]);
        before[input[i]%12]++;after[a[i+1]%12]++;
    }
    assert(!memcmp(before,after,sizeof before));
    if(!inversion && !spread) assert(!memcmp(a+1,input,n));
}

int main(void)
{
    static const uint8_t triad[]={60,64,67},seventh[]={60,64,67,71};
    static const uint8_t octave[]={60,72},sparse[]={60,67,74};
    expect(triad,(uint8_t[]){64,67,72},3,1,0);
    expect(triad,(uint8_t[]){67,72,76},3,2,0);
    expect(triad,(uint8_t[]){72,76,79},3,3,0);
    expect(triad,(uint8_t[]){55,60,64},3,-1,0);
    expect(triad,(uint8_t[]){48,52,55},3,-3,0);
    expect(triad,(uint8_t[]){60,67,76},3,0,1);
    expect(seventh,(uint8_t[]){60,67,76,83},4,0,1);
    expect(triad,(uint8_t[]){48,64,79},3,0,2);
    expect(octave,(uint8_t[]){72,84},2,1,0);
    expect(octave,(uint8_t[]){48,60},2,-1,0);
    expect(sparse,(uint8_t[]){67,74,84},3,1,0);
    expect((uint8_t[]){0,12,24,36},(uint8_t[]){0,24,48,60},4,0,1);
    /* Saturation refuses a whole rotation, rather than clipping its pitch class.
     * WIDE can still move the opposite edge when only one edge is at a bound. */
    expect((uint8_t[]){0,4,7},(uint8_t[]){0,4,7},3,-3,0);
    expect((uint8_t[]){120,124,127},(uint8_t[]){120,124,127},3,3,0);
    expect((uint8_t[]){0,4,7},(uint8_t[]){0,4,19},3,0,2);
    expect((uint8_t[]){120,124,127},(uint8_t[]){108,124,127},3,0,2);
    expect((uint8_t[]){0,108,120},(uint8_t[]){0,108,120},3,0,1);
    expect((uint8_t[]){60},(uint8_t[]){96},1,99,2);
    expect((uint8_t[]){60},(uint8_t[]){24},1,-99,1);
    expect(triad,(uint8_t[]){48,64,79},3,0,UINT32_MAX);
    expect(triad,(uint8_t[]){72,76,79},3,INT32_MAX,0);
    expect(triad,(uint8_t[]){48,52,55},3,INT32_MIN,0);

    uint32_t seed=0xC0ED1234;
    for(uint32_t trial=0;trial<50000;trial++) {
        uint8_t notes[4];uint32_t n=1+trial%4;
        for(uint32_t i=0;i<n;i++) {
            int distinct;
            do {
                seed=seed*1664525u+1013904223u;
                notes[i]=(uint8_t)(seed>>25);distinct=1;
                for(uint32_t j=0;j<i;j++) if(notes[i]==notes[j]) distinct=0;
            } while(!distinct);
        }
        harmony_sort_notes(notes,n);
        for(int32_t inv=-3;inv<=3;inv++) for(uint32_t spread=0;spread<=2;spread++)
            invariant(notes,n,inv,spread);
    }
    uint8_t invalid[]={7,8,9,10,11};
    harmony_shape(invalid,0,1,1);harmony_shape(invalid,5,1,1);
    assert(!memcmp(invalid,(uint8_t[]){7,8,9,10,11},5));
    /* Existing nearest-inversion output remains stable with the shared sort. */
    uint8_t led[]={60,64,67};
    harmony_voice_lead(led,3,(uint8_t[]){55,59,62},3,0);
    assert(!memcmp(led,(uint8_t[]){55,60,64},3));
    puts("chord voicing: 1,050,000 range/identity/pitch-class cases passed");
    return 0;
}
