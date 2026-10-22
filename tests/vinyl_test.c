/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG /* Always execute DSP assertions in optimized builds. */
#include <assert.h>
static void clear_vinyl(void) { memset(&vinyl,0,sizeof vinyl);vinyl.rnd=0x3156494E; }
int main(void)
{
    int32_t l[CTL],r[CTL];host_tracks_init();clear_vinyl();
    for(int i=0;i<CTL;i++)l[i]=r[i]=i*123-4000;
    vinyl_process(l,r,CTL);
    for(int i=0;i<CTL;i++)assert(l[i]==i*123-4000&&r[i]==l[i]);
    /* Tone attenuates high frequencies more strongly than low frequencies. */
    double energy[2]={0};
    for(int f=0;f<2;f++) {
        clear_vinyl();song.g[G_VTONE]=127;
        for(int b=0;b<3000;b++) {
            for(int i=0;i<CTL;i++)l[i]=r[i]=12000*sin((b*CTL+i)*6.283185307*(f?10000:200)/FS);
            vinyl_process(l,r,CTL);
            for(int i=0;i<CTL;i++) {assert(l[i]==r[i]);if(b>1000)energy[f]+=(double)l[i]*l[i];}
        }
    }
    printf("VINYL tone 10kHz / 200Hz power %.4f\n",energy[1]/energy[0]);
    assert(energy[1]<energy[0]*.03);
    /* WOW modulates the signal but keeps equal stereo inputs phase aligned. */
    clear_vinyl();song.g[G_VTONE]=0;song.g[G_VWOW]=127;uint64_t difference=0;
    for(int b=0;b<3000;b++) {
        for(int i=0;i<CTL;i++)l[i]=r[i]=12000*sin((b*CTL+i)*.07);
        int32_t dry=l[0];vinyl_process(l,r,CTL);difference+=abs(l[0]-dry);
        for(int i=0;i<CTL;i++)assert(l[i]==r[i]&&abs(l[i])<13000);
    }
    assert(difference>100000);
    clear_vinyl();song.g[G_VWOW]=0;song.g[G_VCRACKLE]=127;song.playing=1;uint64_t noise=0;
    for(int b=0;b<1000;b++) {memset(l,0,sizeof l);memset(r,0,sizeof r);vinyl_process(l,r,CTL);for(int i=0;i<CTL;i++)noise+=abs(l[i]);}
    assert(noise>10000);song.playing=0;
    for(int b=0;b<200;b++) {memset(l,0,sizeof l);memset(r,0,sizeof r);vinyl_process(l,r,CTL);}
    for(int i=0;i<CTL;i++)assert(!l[i]&&!r[i]);
    /* Live input still receives wow/tone while stopped; overloaded buses stay bounded. */
    song.g[G_VWOW]=song.g[G_VTONE]=127;
    for(int b=0;b<1000;b++) {for(int i=0;i<CTL;i++)l[i]=r[i]=((i+b)&1)?4000000:-4000000;vinyl_process(l,r,CTL);for(int i=0;i<CTL;i++)assert(abs(l[i])<=4100000&&l[i]==r[i]);}
    song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=0;
    for(int b=0;b<1100;b++) {for(int i=0;i<CTL;i++)l[i]=r[i]=1234;vinyl_process(l,r,CTL);}
    for(int i=0;i<CTL;i++)assert(l[i]==1234&&r[i]==1234);
    /* Pitch excursion must be audible, not merely different samples from delay.
     * Fractional rising crossings measure local pitch without a DSP-state probe. */
    host_tracks_init();clear_vinyl();song.g[G_VWOW]=127;
    double last=-1,previous=-1,min_hz=1e9,max_hz=0;
    int32_t sample_before=0;
    for(uint32_t b=0;b<FS*4u/CTL;b++) {
        for(uint32_t i=0;i<CTL;i++)l[i]=r[i]=12000*sin((b*CTL+i)*6.283185307*880/FS);
        vinyl_process(l,r,CTL);
        for(uint32_t i=0;i<CTL;i++) {
            uint32_t t=b*CTL+i;
            if(sample_before<0 && l[i]>=0) {
                last=t-1+(double)-sample_before/(l[i]-sample_before);
                if(t>FS && previous>0) {double hz=FS/(last-previous);if(hz<min_hz)min_hz=hz;if(hz>max_hz)max_hz=hz;}
                previous=last;
            }
            sample_before=l[i];
        }
    }
    printf("VINYL 880Hz wow range %.2f..%.2f Hz\n",min_hz,max_hz);
    assert(min_hz<872 && max_hz>888 && min_hz>840 && max_hz<920);
    /* End-to-end silence through the real master chain: full crackle should be
     * audible under music, while keeping ample output headroom. */
    host_tracks_init();clear_vinyl();song.g[G_LTYPE]=1;song.playing=1;song.g[G_VCRACKLE]=127;
    double ss=0;uint32_t count=0,quiet=0,positive=0,negative=0;int peak=0;int32_t out[CTL*2];
    for(uint32_t b=0;b<FS*4u/CTL;b++) {
        mix_block(out,CTL);
        if(b>FS/CTL)for(uint32_t i=0;i<CTL*2;i++){ss+=(double)out[i]*out[i];count++;quiet+=abs(out[i])<100;positive+=out[i]>3000;negative+=out[i]<-3000;if(abs(out[i])>peak)peak=abs(out[i]);}
    }
    double rms=sqrt(ss/count);printf("VINYL crackle RMS %.1f dBFS, peak %.1f dBFS\n",20*log10(rms/32768),20*log10(peak/32768.));
    printf("VINYL crackle crest %.2f, quiet fraction %.4f\n",peak/rms,(double)quiet/count);
    assert(rms>328 && rms<1305 && peak>3000 && peak<16000);
    assert(peak/rms>12 && (double)quiet/count>.90);
    assert(positive>10 && negative>10);
    /* The former MONO key is now LOFI: effective settings are temporary,
     * stereo stays stereo, and releasing it returns to saved parameters. */
    host_tracks_init();punch_clear();clear_vinyl();song.playing=0;
    song.g[G_VWOW]=10;song.g[G_VTONE]=20;song.g[G_VCRACKLE]=30;
    punch_hold(0,PX_VINYL,1);assert(punch_vinyl_requested());
    uint64_t stereo=0;
    for(uint32_t b=0;b<200;b++) {
        for(uint32_t i=0;i<CTL;i++){l[i]=12000*sin((b*CTL+i)*.11);r[i]=-l[i];}
        vinyl_process(l,r,CTL);
        for(uint32_t i=0;i<CTL;i++)stereo+=abs(l[i]-r[i]);
    }
    assert(stereo>1000000 && vinyl.wow==96*256 && vinyl.tone==90*256 && vinyl.crackle==100*256);
    assert(song.g[G_VWOW]==10 && song.g[G_VTONE]==20 && song.g[G_VCRACKLE]==30);
    punch_hold(0,PX_VINYL,0);assert(!punch_vinyl_requested());
    for(uint32_t b=0;b<200;b++){for(uint32_t i=0;i<CTL;i++)l[i]=r[i]=1234;vinyl_process(l,r,CTL);}
    assert(vinyl.wow==10*256 && vinyl.tone==20*256 && vinyl.crackle==30*256);
    song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=0;
    for(uint32_t b=0;b<200;b++){for(uint32_t i=0;i<CTL;i++)l[i]=r[i]=1234;vinyl_process(l,r,CTL);}
    for(uint32_t i=0;i<CTL;i++)assert(l[i]==1234 && r[i]==1234);
    /* MIDI-held punch goes through the production mixer, including event routing. */
    host_tracks_init();punch_clear();clear_vinyl();song.playing=1;
    punch_hold(2,PX_VINYL,1);uint64_t heard=0;
    for(uint32_t b=0;b<200;b++){mix_block(out,CTL);for(uint32_t i=0;i<CTL*2;i++)heard+=abs(out[i]);}
    assert(heard>100000 && !song.g[G_VWOW] && !song.g[G_VTONE] && !song.g[G_VCRACKLE]);
    punch_hold(2,PX_VINYL,0);
    for(uint32_t b=0;b<200;b++)mix_block(out,CTL);
    printf("VINYL released mixer tail %d / %d, state %d %d %d %d\n",out[0],out[1],vinyl.wow,vinyl.wet,vinyl.tone,vinyl.crackle);
    assert(!vinyl.wow && !vinyl.wet && !vinyl.tone && !vinyl.crackle);
    /* The master DC blocker can retain a one-LSB release tail; VINYL is bypassed. */
    for(uint32_t i=0;i<CTL*2;i++)assert(abs(out[i])<=2);
    /* Production UI-to-DSP mapping must keep TAPE/VHS/CD selected during punch,
     * and high saved settings must never be reduced to the momentary floors. */
    for(unsigned mode=VINYL_MODE_TAPE;mode<=VINYL_MODE_CD;mode++) {
        host_tracks_init();punch_clear();clear_vinyl();
        song.g[G_LTYPE]=mode+1;
        song.g[G_VWOW]=127;song.g[G_VTONE]=121;song.g[G_VCRACKLE]=115;
        punch_hold(2,PX_VINYL,1);
        for(uint32_t b=0;b<200;b++)mix_block(out,CTL);
        assert(vinyl.mode==mode && song.g[G_LTYPE]==mode+1);
        assert(vinyl.wow==127*256 && vinyl.tone==121*256 && vinyl.crackle==115*256);
        punch_hold(2,PX_VINYL,0);
        for(uint32_t b=0;b<200;b++)mix_block(out,CTL);
        assert(vinyl.mode==mode && song.g[G_LTYPE]==mode+1);
        assert(vinyl.wow==127*256 && vinyl.tone==121*256 && vinyl.crackle==115*256);
    }
    puts("VINYL: bypass, frequency response, wow/stereo, PLAY/STOP noise, hot input and exit PASS");return 0;
}
