/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(int argc,char **argv)
{
    host_tracks_init();
    for(uint32_t i=0;i<DLY_LEN;i++)dly_buf[i]=(int16_t)(i*97u);
    static const uint32_t lengths[]={16,31,1024,11025,32768,65535};
    for(uint32_t l=0;l<NELEM(lengths);l++)for(uint32_t p=0;p<65536;p+=17) {
        uint32_t len=lengths[l]>32768?32768:lengths[l],writer=19;
        reverse_delay.phase=p<<16;
        int32_t samples[2];
        for(uint32_t h=0;h<2;h++) {
            uint32_t phase=(p+h*32768u)%65536u;
            uint32_t age=1+2*(uint32_t)((uint64_t)phase*len/65536u);
            assert(age>0 && age<DLY_LEN);
            samples[h]=dly_buf[(writer-age)%DLY_LEN];
        }
        int32_t weight=p<32768?p:65535-p;
        int32_t want=samples[1]+(int32_t)(((int64_t)(samples[0]-samples[1])*weight)>>15);
        assert(delay_reverse(lengths[l],writer)==want);
        reverse_delay.mix=0;fx.dly_w=writer;
        assert(delay_read(lengths[l])==dly_buf[(writer-lengths[l])%DLY_LEN]);
        reverse_delay.mix=32768;assert(delay_read(lengths[l])==want);
    }
    memset(dly_buf,0,sizeof dly_buf);memset(&fx,0,sizeof fx);
    memset(&reverse_delay,0,sizeof reverse_delay);memset(&delay_age,0,sizeof delay_age);
    song.g[G_DMIX]=127;song.g[G_DFDBK]=127;song.g[G_DCOLOR]=100;
    FILE *wav=argc>1?fopen(argv[1],"wb"):0;if(wav)wav_hdr(wav,0);
    uint64_t energy=0;int32_t in[CTL],zero[CTL]={0},left[CTL],right[CTL];uint32_t total=0;
    for(uint32_t b=0;b<12000;b++) {
        song.g[G_DTYPE]=b<8000?4:(b/160)%5;
        song.g[G_DTIME]=b<8000?1:(b/80)%9;
        for(uint32_t i=0;i<CTL;i++) {
            uint32_t pos=b*CTL+i,beat=pos%(FS/2);
            in[i]=beat<FS/8 ? (int32_t)(12000*sin(2*M_PI*440*pos/FS))*(int32_t)(FS/8-beat)/(FS/8):0;
        }
        fx_buses(zero,in,zero,left,right,CTL);
        for(uint32_t i=0;i<CTL;i++) {
            assert(left[i]>=-65536 && left[i]<=65536 && left[i]==right[i]);
            energy+=llabs(left[i]);if(wav)wav_put(wav,left[i]/2,right[i]/2);
        }
        total+=CTL;
    }
    assert(energy>1000000);
    if(wav){fseek(wav,0,SEEK_SET);wav_hdr(wav,total);fclose(wav);}
    puts("Reverse delay: window reference, ring safety, dry identity, mode/time transitions and max-feedback stability PASS");
    return 0;
}
