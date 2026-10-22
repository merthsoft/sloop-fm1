/* Offline comparison of the actual fx_buses path in the private snapshot.
 * Synthetic input, no normalization, no master LOFI, no hardware capture. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
int main(int argc,char **argv)
{
    assert(argc==3);unsigned mode=(unsigned)atoi(argv[1]);host_tracks_init();
    /* All int16 tap values and live mode transitions are bounded. */
    for(unsigned m=0;m<3;m++) {
        memset(&delay_age,0,sizeof delay_age);
        for(unsigned i=0;i<512;i++)delay_character_sample(0,m,i);
        for(int x=-32768;x<32768;x++) {
            int y=delay_character_sample(x,m,(unsigned)x);
            assert(y>=-32768&&y<=32767);
            if(!m)assert(y==x);
            if(m==2)assert(y%256==0);
        }
    }
    /* Independent controls: DRIVE zero is clean; all CRUSH rates/resolutions are bounded. */
    song.g[G_DDRIVE]=0;memset(&delay_age,0,sizeof delay_age);
    for(int x=-32768;x<32768;x++)assert(delay_character_sample(x,1,(unsigned)x)==x);
    song.g[G_DDRIVE]=127;
    for(int bits=4;bits<=16;bits+=4)for(int rate=0;rate<=4;rate++) {
        song.g[G_DBITS]=bits;song.g[G_DHOLD]=rate;memset(&delay_age,0,sizeof delay_age);
        for(unsigned i=0;i<512;i++)delay_character_sample(0,2,i);
        int quantum=1<<(16-bits);
        for(int x=-32768;x<32768;x++) {
            int y=delay_character_sample(x,2,(unsigned)x);
            assert(y>=-32768&&y<=32767&&y%quantum==0);
            if(bits==16&&rate==4)assert(y==x);
        }
    }
    song.g[G_DBITS]=8;song.g[G_DHOLD]=2;
    memset(&delay_age,0,sizeof delay_age);
    int prev=12000;
    for(unsigned i=0;i<10000;i++) {
        int y=delay_character_sample(12000,(i/1000)%3,i);
        assert(abs(y-prev)<=64);prev=y;
    }
    for(unsigned i=0;i<1000;i++)assert(delay_character_sample(0,2,i)>=-32768);
    assert(delay_character_sample(0,2,1000)==0);
    memset(&delay_age,0,sizeof delay_age);song.g[G_DTYPE]=mode;
    song.g[G_BPM]=120;song.g[G_DTIME]=1;song.g[G_DFDBK]=100;song.g[G_DCOLOR]=100;song.g[G_DMIX]=127;
    unsigned delay=delay_samples(),burst=FS/8,frames=FS*4/CTL*CTL;
    double energy[8]={0};unsigned counts[8]={0};
    FILE *f=fopen(argv[2],"wb");assert(f);wav_hdr(f,frames);
    for(unsigned base=0;base<frames;base+=CTL) {
        int32_t zero[CTL]={0},send[CTL],l[CTL]={0},r[CTL]={0};
        for(unsigned i=0;i<CTL;i++) {
            unsigned t=base+i;double sec=(double)t/FS;
            send[i]=t<burst?(int32_t)(36000*exp(-sec/.04)*(.55*sin(6.283185307*220*sec)+.25*sin(6.283185307*880*sec)+.20*sin(6.283185307*6000*sec))):0;
        }
        fx_buses(zero,send,zero,l,r,CTL);
        for(unsigned i=0;i<CTL;i++) {
            unsigned t=base+i;assert(l[i]==r[i]);assert(abs(l[i])<=65536);wav_put(f,l[i],r[i]);
            for(unsigned n=1;n<=8;n++)if(t>=n*delay&&t<n*delay+burst){energy[n-1]+=(double)l[i]*l[i];counts[n-1]++;}
        }
    }
    assert(!fclose(f));
    /* Full send/feedback and simultaneous character/time/tempo changes stay bounded. */
    song.g[G_DFDBK]=127;
    for(unsigned b=0;b<1200;b++) {
        int32_t zero[CTL]={0},send[CTL],l[CTL]={0},r[CTL]={0};
        song.g[G_DDRIVE]=b%128;song.g[G_DBITS]=4+(b%13);song.g[G_DHOLD]=b%5;
        song.g[G_DTYPE]=(b/29)%3;song.g[G_DTIME]=(b/17)%(GP[G_DTIME].max+1);
        song.g[G_BPM]=40+(b%201);song.g[G_DCOLOR]=b%128;
        for(unsigned i=0;i<CTL;i++)send[i]=b<600?((b+i)&1?65534:-65536):0;
        fx_buses(zero,send,zero,l,r,CTL);
        for(unsigned i=0;i<CTL;i++)assert(l[i]==r[i]&&abs(l[i])<=65536);
        assert(fx.dly_fade<=64&&fx.dly_time>0&&fx.dly_time<DLY_LEN);
    }
    printf("mode=%u tap=%u RMS",mode,delay);
    for(unsigned n=0;n<8;n++)printf(" %.2f",sqrt(energy[n]/counts[n]));puts("");return 0;
}
