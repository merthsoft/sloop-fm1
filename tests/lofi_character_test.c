/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>
static void reset(unsigned mode) {
    host_tracks_init(); punch_clear(); memset(&vinyl,0,sizeof vinyl);
    vinyl.mode=mode; song.playing=1;
}
static void put32(FILE *f,uint32_t n) {for(int i=0;i<4;i++)fputc(n>>(8*i),f);}
static void put16(FILE *f,uint32_t n) {fputc(n,f);fputc(n>>8,f);}
/* Punch must equal a normal render of the selected character with parameter
 * floors, not merely emit nonzero audio (which could hide forced VINYL). */
static void check_punch_characters(void)
{
    uint64_t signatures[VINYL_MODE_COUNT]={0};
    for(unsigned mode=0;mode<VINYL_MODE_COUNT;mode++) for(unsigned strong=0;strong<2;strong++) {
        unsigned effective=mode==VINYL_MODE_OFF?VINYL_MODE_VINYL:mode;
        uint64_t hash[2]={0};
        for(unsigned punched=0;punched<2;punched++) {
            reset(mode);
            song.g[G_LTYPE]=mode==VINYL_MODE_OFF?0:mode>=VINYL_MODE_RADIO?mode:mode+1;
            song.g[G_VWOW]=strong?127:punched?25:96;
            song.g[G_VTONE]=strong?121:punched?35:90;
            song.g[G_VCRACKLE]=strong?115:punched?45:100;
            int16_t saved[G_COUNT];memcpy(saved,song.g,sizeof saved);
            if(punched) punch_hold(0,PX_VINYL,1);
            int32_t l[CTL],r[CTL];
            hash[punched]=1469598103934665603ull;
            for(uint32_t b=0;b<300;b++) {
                for(uint32_t i=0;i<CTL;i++) {
                    uint32_t t=b*CTL+i;
                    l[i]=(int32_t)(10000*sin(t*.031)+3000*sin(t*.39));
                    r[i]=-l[i];
                }
                vinyl_process_mode(l,r,CTL,punched?mode:effective);
                for(uint32_t i=0;i<CTL;i++)
                    hash[punched]=(hash[punched]^(uint32_t)l[i])*1099511628211ull;
            }
            assert(!memcmp(saved,song.g,sizeof saved));
            assert(vinyl.mode==effective);
            assert(vinyl.wow==(mode==VINYL_MODE_RADIO?(strong?127:120):strong?127:96)*256);
            assert(vinyl.tone==(strong?121:90)*256);
            assert(vinyl.crackle==(strong?115:100)*256);
            if(punched) {
                punch_hold(0,PX_VINYL,0);
                /* Each one-sample release follows the existing slew bounds. */
                for(uint32_t i=0;i<1400;i++) {
                    int32_t wow=vinyl.wow,tone=vinyl.tone,crackle=vinyl.crackle;
                    l[0]=r[0]=1234;
                    vinyl_process_mode(l,r,1,mode);
                    assert(abs(vinyl.wow-wow)<=32 && abs(vinyl.tone-tone)<=32 && abs(vinyl.crackle-crackle)<=32);
                }
                for(uint32_t b=0;b<300;b++) {
                    for(uint32_t i=0;i<CTL;i++){l[i]=1234;r[i]=-2345;}
                    vinyl_process_mode(l,r,CTL,mode);
                }
                assert(!memcmp(saved,song.g,sizeof saved));
                assert(vinyl.wow==(mode==VINYL_MODE_OFF?0:mode==VINYL_MODE_RADIO?((saved[G_VWOW]*(256-saved[G_VWOW]))>>7)*256:saved[G_VWOW]*256));
                assert(vinyl.tone==(mode==VINYL_MODE_OFF?0:saved[G_VTONE]*256));
                assert(vinyl.crackle==(mode==VINYL_MODE_OFF?0:saved[G_VCRACKLE]*256));
                if(mode==VINYL_MODE_OFF) for(uint32_t i=0;i<CTL;i++)assert(l[i]==1234&&r[i]==-2345);
            }
        }
        assert(hash[0]==hash[1]);
        if(!strong && mode<VINYL_MODE_COUNT) signatures[mode]=hash[1];
    }
    for(unsigned a=0;a<VINYL_MODE_COUNT;a++) for(unsigned b=a+1;b<VINYL_MODE_COUNT;b++)
        if(a!=VINYL_MODE_OFF && b!=VINYL_MODE_OFF) assert(signatures[a]!=signatures[b]);
}
int main(void) {
    check_punch_characters();
    int32_t l[CTL],r[CTL]; double signature[VINYL_MODE_COUNT]={0};
    for(unsigned mode=0;mode<VINYL_MODE_COUNT;mode++) {
        if(mode==VINYL_MODE_OFF)continue;
        reset(mode);song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=100;
        char path[128];snprintf(path,sizeof path,"build/lofi-master/character-%u.wav",mode);
        FILE *f=fopen(path,"wb");assert(f);uint32_t frames=(FS*5/CTL)*CTL;
        fwrite("RIFF",1,4,f);put32(f,36+frames*4);fwrite("WAVEfmt ",1,8,f);put32(f,16);put16(f,1);put16(f,2);put32(f,FS);put32(f,FS*4);put16(f,4);put16(f,16);fwrite("data",1,4,f);put32(f,frames*4);
        int peak=0;
        for(uint32_t b=0;b<frames/CTL;b++) {
            for(uint32_t i=0;i<CTL;i++) {double t=(double)(b*CTL+i)/FS;
                l[i]=r[i]=(int32_t)(9000*sin(t*6.283185307*220)+4000*sin(t*6.283185307*1760)+2000*sin(t*6.283185307*4400));}
            vinyl_process_mode(l,r,CTL,mode);
            for(uint32_t i=0;i<CTL;i++) {assert(l[i]==r[i]);assert(abs(l[i])<40000);if(abs(l[i])>peak)peak=abs(l[i]);signature[mode]+=(double)l[i]*l[i];put16(f,clamp(l[i],-32768,32767));put16(f,clamp(r[i],-32768,32767));}
        } fclose(f);printf("mode %u RMS %.1f peak %d\n",mode,sqrt(signature[mode]/frames),peak);
        /* Stopped synthetic noise settles, but live stereo remains independent. */
        song.playing=0;
        for(int b=0;b<600;b++){memset(l,0,sizeof l);memset(r,0,sizeof r);vinyl_process_mode(l,r,CTL,mode);}
        for(int i=0;i<CTL;i++)assert(l[i]==0&&r[i]==0);
        for(int b=0;b<100;b++){for(int i=0;i<CTL;i++){l[i]=4000000;r[i]=-4000000;}vinyl_process_mode(l,r,CTL,mode);for(int i=0;i<CTL;i++)assert(abs(l[i])<=4100000&&abs(r[i])<=4100000);}
        song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=0;
        for(int b=0;b<600;b++){for(int i=0;i<CTL;i++){l[i]=1234;r[i]=-2345;}vinyl_process_mode(l,r,CTL,mode);}
        for(int i=0;i<CTL;i++)assert(l[i]==1234&&r[i]==-2345);
    }
    for(int a=0;a<4;a++)for(int b=a+1;b<4;b++)assert(fabs(signature[a]-signature[b])>signature[a]*.01);
    /* Every mode transition preserves constant content without a gain hole. */
    reset(0);song.playing=0;song.g[G_VWOW]=song.g[G_VTONE]=90;
    for(unsigned m=0;m<12;m++)for(int b=0;b<600;b++) {
        for(int i=0;i<CTL;i++)l[i]=r[i]=10000;
        vinyl_process_mode(l,r,CTL,m%4);
        if(m || b>100)for(int i=0;i<CTL;i++)assert(abs(l[i]-10000)<20&&l[i]==r[i]);
    }
    /* CD captures a 32-frame fragment and repeats it during LATCH. This tests
     * actual repetition, rather than accepting a mute/quantizer as a skip. */
    reset(VINYL_MODE_CD);song.g[G_VTONE]=song.g[G_VCRACKLE]=127;
    int32_t history[32]={0};int repeated=0,changed=0;
    for(int t=0;t<4000;t++) {
        int32_t dry=(t*23)%20000-10000;l[0]=dry;r[0]=-dry;
        vinyl_process_mode(l,r,1,VINYL_MODE_CD);
        if(t>1024 && vinyl.cd_count>32 && vinyl.cd_count<224) {
            assert(abs(l[0]-history[t&31])<=2);repeated++;
            changed+=abs(l[0]-dry)>100;
            assert(abs(l[0]+r[0])<=2);
        }
        history[t&31]=l[0];
    }
    assert(repeated>1000 && changed>1000);
    printf("CD repeated-fragment samples %d; changed from dry %d\n",repeated,changed);
    /* OFF ramps to exact dry while retaining saved knob amounts for re-enable. */
    song.playing=1;song.g[G_VWOW]=song.g[G_VTONE]=song.g[G_VCRACKLE]=127;
    for(int b=0;b<600;b++) {
        for(int i=0;i<CTL;i++){l[i]=1234;r[i]=-2345;}
        vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);
    }
    for(int i=0;i<CTL;i++)assert(l[i]==1234&&r[i]==-2345);
    assert(song.g[G_VWOW]==127&&song.g[G_VTONE]==127&&song.g[G_VCRACKLE]==127);
    /* Momentary VINYL still works over OFF, then returns to dry. */
    song.g[G_LTYPE]=0;
    punch_hold(0,PX_VINYL,1);uint64_t punched=0;
    for(int b=0;b<100;b++) {
        for(int i=0;i<CTL;i++)l[i]=r[i]=((b*CTL+i)*123)%20000-10000;
        int32_t dry=l[0];vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);punched+=abs(l[0]-dry);
    }
    assert(punched>10000&&song.g[G_LTYPE]==0);
    punch_hold(0,PX_VINYL,0);
    for(int b=0;b<600;b++) {
        for(int i=0;i<CTL;i++){l[i]=1234;r[i]=-2345;}
        vinyl_process_mode(l,r,CTL,VINYL_MODE_OFF);
    }
    for(int i=0;i<CTL;i++)assert(l[i]==1234&&r[i]==-2345);
    puts("LOFI characters: stereo, bounded overload, silence, exact bypass, transitions and distinct responses PASS");
}
