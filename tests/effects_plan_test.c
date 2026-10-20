/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
static void half_transport(void)
{
    for (int bpm=40;bpm<=240;bpm+=1) {
        host_tracks_init(); punch_clear(); song.g[G_BPM]=bpm; song.g[G_SYNC]=0;
        song.playing=1; clk_pos=clk_beat=0;
        uint64_t expected=0; uint32_t rem=0;
        for (int i=0;i<400;i++) {
            uint32_t n=1u+(i%CTL); punch.req=(i%7<4)?PX_HALF:-1;
            uint32_t u=n*bpm;
            if(punch.req==PX_HALF) {u+=rem;rem=u&1u;u>>=1;}
            expected+=u; events_block(n);
            assert((uint64_t)clk_beat*BEAT_U+clk_pos==expected);
            assert(song.g[G_BPM]==bpm);
        }
        song.g[G_SYNC]=1; punch.req=PX_HALF; uint32_t before=clk_pos;
        events_block(CTL); assert(!musical_half); assert(clk_pos-before==CTL*(uint32_t)bpm);
        song.g[G_SYNC]=0; punch.req=PX_HALF; events_block(CTL); assert(musical_half);
        seq_stop(); assert(!musical_half&&!punch_half_requested());
        events_block(CTL); assert(!song.playing);
    }
    host_tracks_init(); musical_half=1; song.g[G_BPM]=121;
    assert(musical_samples(BEAT_U)==BEAT_U*2u/121u);
    assert(div_samples(0)==div_units(0)*2u/121u);
    musical_half=0;
    puts("PASS HALF: exact fractional transport at 40..240 BPM, external clock bypass, unchanged saved BPM, STOP cleanup");
}
static void half_clock_and_slicer(void)
{
    host_tracks_init();punch_clear();song.g[G_BPM]=121;song.g[G_SYNC]=0;song.g[G_CLKOUT]=1;
    usb.config=1;mo_r=mo_w=0;song.playing=1;mco_play=0;mco_u=0;
    uint32_t f8=0,fa=0,fc=0;uint64_t units=0;uint8_t rem=0;
    for(int b=0;b<20000;b++) {
        musical_half=b<7000?1:b<14000?2:0;uint32_t n=1u+b%CTL;
        units+=musical_units(n,&rem);clkout_block(n);
        while(mo_r!=mo_w) {uint32_t st=(midi_out_q[mo_r++%MOUT_Q]>>8)&255u;f8+=st==0xF8u;fa+=st==0xFAu;fc+=st==0xFCu;}
    }
    assert(fa==1&&fc==0&&f8==1u+units/MCLK_PULSE_U);
    /* Per-track slicer phase uses the same exact fractional rate, even for odd BPM. */
    song.playing=0; musical_half=1; slicer_start(0);uint64_t elapsed=0;rem=0;
    trk[0].p[P_SLRATE]=1; song.g[G_SWING]=0;trk[0].p[P_SSWING]=0;
    for(int b=0;b<30000;b++) {
        musical_half=b<10000?1:b<20000?2:0;
        uint32_t n=1u+b%CTL;elapsed+=musical_units(n,&rem);slicer_track(&trk[0],0,n);
        uint32_t len=BEAT_U/SL_DEN[1];
        assert(((uint64_t)sl[0].idx*len+sl[0].pos)%(16u*len)==elapsed%(16u*len));
    }
    musical_half=0;punch_clear();
    host_tracks_init();punch_clear();song.playing=1;song.g[G_BPM]=121;song.g[G_SYNC]=0;
    punch.req=PX_HALF;events_block(31);uint32_t before=clk_pos;
    song.g[G_BPM]=99;events_block(32);assert(clk_pos-before==1584u&&song.g[G_BPM]==99);
    punch.req=-1;before=clk_pos;events_block(32);assert(clk_pos-before==3168u);
    punch_clear();song.playing=0;
    puts("PASS HALF/DOUBLE: exact outgoing MIDI clock, no extra START/STOP, exact per-track slicer phase at odd BPM");
}
static void double_transport(void)
{
    for(int bpm=40;bpm<=240;bpm++) {
        host_tracks_init();punch_clear();song.g[G_BPM]=bpm;song.g[G_SYNC]=0;
        song.playing=1;clk_pos=clk_beat=0;
        uint64_t expected=0;
        for(int i=0;i<1000;i++) {
            uint32_t n=1u+i%CTL;
            punch_hold(2,PX_DOUBLE,i%3!=0);
            expected+=(uint64_t)n*bpm*(i%3!=0?2u:1u);
            events_block(n);
            assert((uint64_t)clk_beat*BEAT_U+clk_pos==expected && song.g[G_BPM]==bpm);
        }
        punch_clear();punch_hold(0,PX_HALF,1);assert(punch_rate_requested()==1);
        punch_hold(0,PX_DOUBLE,1);assert(punch_rate_requested()==2);
        punch_hold(0,PX_DOUBLE,0);assert(punch_rate_requested()==1);
        punch_hold(0,PX_HALF,0);assert(!punch_rate_requested());
        punch_hold(1,PX_HALF,1);punch_hold(0,PX_DOUBLE,1);assert(punch_rate_requested()==2);
        punch_hold(2,PX_HALF,1);assert(punch_rate_requested()==2);
        punch_hold(0,PX_DOUBLE,0);assert(punch_rate_requested()==1);
        punch_clear();punch_hold(2,PX_DOUBLE,1);song.g[G_SYNC]=1;
        events_block(CTL);assert(!musical_half);
        song.g[G_SYNC]=0;events_block(CTL);assert(musical_half==2);
        assert(musical_samples(BEAT_U)==BEAT_U/(2u*bpm));
        seq_stop();assert(!musical_half&&!punch_rate_requested());
    }
    puts("PASS DOUBLE: 40..240 BPM exact phase, held fallback, source priority, external bypass, STOP and saved BPM");
}
static void combination_holds(void)
{
    host_tracks_init(); punch_clear(); song.g[G_SYNC]=0;
    punch_hold(0,PX_REV,1); punch_hold(0,PX_LPF,1); punch_hold(0,PX_GATE,1); punch_hold(0,PX_HALF,1);
    assert(punch.req==PX_REV&&punch_filter_req==PX_LPF&&punch_gate_req&&punch_half_requested());
    punch_hold(0,PX_DOUBLE,1); assert(punch.req==PX_REV && punch_rate_requested()==2);
    punch_hold(0,PX_DOUBLE,0); assert(punch.req==PX_REV);
    punch_hold(0,PX_HPF,1); assert(punch_filter_req==PX_HPF);
    punch_hold(0,PX_HPF,0); assert(punch_filter_req==PX_LPF);
    punch_hold(1,PX_REV,1); punch_hold(0,PX_REV,0); assert(punch.req==PX_REV);
    punch_hold(0,PX_DOUBLE,1); punch_hold(1,PX_DOUBLE,0); assert(punch.req==PX_REV && punch_rate_requested()==2);
    punch_midi_clear(); assert(punch.req<0 && punch_rate_requested()==2);
    punch_clear();punch_hold(1,PX_REV,1);punch_hold(2,PX_REV,1);punch_midi_clear();
    assert(punch.req==PX_REV&&punch_rank[2][PX_REV]);
    punch_hold(1,PX_REV,0);assert(punch.req==PX_REV);punch_hold(2,PX_REV,0);assert(punch.req<0);
    punch_clear(); assert(!punch_local&&!punch_gate_req&&punch_filter_req<0&&!punch_half_requested());
    int ring[]={PX_LOOP8,PX_REV,PX_STOP,PX_FLANGE,PX_ECHO};
    for(unsigned k=0;k<sizeof ring/sizeof ring[0];k++)for(int filter=PX_LPF;filter<=PX_TEL;filter++) {
        punch_clear(); memset(&punch,0,sizeof punch);punch.req=punch.cur=-1;
        punch_filter.cur=-1;punch_filter.gain=0;punch_gate.gain=0;
        punch_hold(0,ring[k],1);punch_hold(0,filter,1);punch_hold(0,PX_GATE,1);
        int32_t l[CTL],rr[CTL];uint32_t before=punch.w;
        for(int b=0;b<1800;b++) {
            for(int i=0;i<CTL;i++) l[i]=(b+i)&1?230000:-230000,rr[i]=-l[i];
            punch_process(l,rr,CTL);
            for(int i=0;i<CTL;i++)assert(abs(l[i])<(1<<22)&&abs(rr[i])<(1<<22));
        }
        assert(punch.w-before==1800u*CTL); /* exactly one ring write pass */
        punch_hold(0,filter,0);assert(punch.req==ring[k]&&punch_gate_req);
        punch_hold(0,ring[k],0);assert(punch_gate_req);
        punch_hold(0,PX_GATE,0);
        for(int b=0;b<10;b++) {for(int i=0;i<CTL;i++)l[i]=12345,rr[i]=-6789;punch_process(l,rr,CTL);}
        for(int i=0;i<CTL;i++)assert(l[i]==12345&&rr[i]==-6789);
    }
    /* Source removal must compact ranks; long sessions cannot wrap priorities. */
    punch_clear();punch_hold(0,PX_REV,1);
    for(int cycle=0;cycle<1000;cycle++) {
        punch_hold(1,PX_DOUBLE,1);punch_hold(0,PX_REV,1);punch_midi_clear();
        assert(punch.req==PX_REV&&punch_rank[0][PX_REV]==1);
    }
    /* Every two-effect release order, repeated presses and source overlaps. */
    for(int a=0;a<PUNCH_NFX;a++)for(int b=0;b<PUNCH_NFX;b++)for(int source=0;source<3;source++) {
        punch_clear();punch_hold(0,a,1);punch_hold(source,b,1);punch_hold(0,a,0);
        if(a!=b||source)assert(punch_rank[source][b]);
        punch_hold(source,b,0);assert(!punch_local);
    }
    puts("PASS combinations: ownership/fallback, every key pair, 15 maximal DSP chains, bounded hot input, one ring pass, exact dry recovery");
}
static void routed_fx_sources(void)
{
    host_tracks_init();punch_clear();song.g[G_FXCH]=1;song.g[G_ROUTE]=0;usb.detached=0;usb.config=1;
    events_block(CTL);events_block(CTL); /* settle channel routing/panic ownership */
    uint32_t note=53u+punch_key_of_fx(PX_REV);
    midi_in_q[mi_w++%MQ]=0x09u|0x90u<<8|note<<16|100u<<24;
    midi_in_q[mi_w++%MQ]=0x19u|0x90u<<8|note<<16|100u<<24;
    events_block(CTL);assert(punch_rank[1][PX_REV]&&punch_rank[2][PX_REV]);
    usb.resets++;events_block(CTL);assert(!punch_rank[1][PX_REV]&&punch_rank[2][PX_REV]);
    midi_in_q[mi_w++%MQ]=0x18u|0x80u<<8|note<<16;
    events_block(CTL);assert(!punch_local);
    ly_bit[LY_FX]=1u<<2;fm1_in.buttons=ly_bit[LY_FX];fm1_in.notes=1u<<punch_key_of_fx(PX_REV);
    events_block(CTL);assert(punch_rank[0][PX_REV]);
    fm1_in.buttons=0;events_block(CTL);assert(!punch_rank[0][PX_REV]);
    fm1_in.notes=0;events_block(CTL);punch_clear();
    puts("PASS real FX packets: USB/TRS same-note ownership, USB reset, TRS release, physical layer exit");
}
static void spring_reverb(void)
{
    host_tracks_init(); punch_clear(); rev_frz=0; song.g[G_RMODEL]=1;
    int32_t in[CTL]={0},zero[CTL]={0},l[CTL],rr[CTL];
    for(int i=0;i<6;i++)fx_buses(zero,zero,zero,l,rr,CTL);
    assert(reverb_model.cur==1&&reverb_model.gain==32768);
    for(int size=0;size<=127;size+=127)for(int damp=0;damp<=127;damp+=127) {
        memset(&spring,0,sizeof spring);memset(rev_line,0,sizeof rev_line);memset(&rev_storage,0,sizeof rev_storage);
        song.g[G_RSIZE]=size;song.g[G_RDAMP]=damp;
        uint64_t energy=0,late=0;int peak=0;
        for(int b=0;b<(int)(FS*8u/CTL);b++) {
            memset(in,0,sizeof in);if(!b)in[0]=120000;
            fx_buses(zero,zero,in,l,rr,CTL);
            for(int i=0;i<CTL;i++) {
                int a=abs(l[i])+abs(rr[i]);if(a>peak)peak=a;
                if(b<FS/CTL)energy+=a;
                if(b>FS*7u/CTL)late+=a;
                assert(a<(1<<22));
            }
        }
        assert(energy>1000&&late<energy/10u);
        printf("SPRING size %d damp %d: peak %d, late/early %.5f\n",size,damp,peak,(double)late/energy);
    }
    for(int b=0;b<2000;b++) {
        for(int i=0;i<CTL;i++)in[i]=(b+i)&1?(1<<22):-(1<<22);
        fx_buses(zero,zero,in,l,rr,CTL);
        for(int i=0;i<CTL;i++)assert(abs(l[i])+abs(rr[i])<(1<<22));
    }
    /* FREEZE holds a fixed delay losslessly after capture; defer model changes. */
    for(int b=0;b<100;b++) { for(int i=0;i<CTL;i++)in[i]=10000*sin((b*CTL+i)*.073);fx_buses(zero,zero,in,l,rr,CTL); }
    rev_frz=1; song.g[G_RMODEL]=0;uint64_t first=0,last=0;
    for(int b=0;b<(int)(FS*12u/CTL);b++) {
        fx_buses(zero,zero,zero,l,rr,CTL);assert(reverb_model.cur==1);
        for(int i=0;i<CTL;i++) {int a=abs(l[i])+abs(rr[i]);assert(a<(1<<22));if(b<FS/CTL)first+=a;if(b>FS*11u/CTL)last+=a;}
    }
    assert(first>1000&&last>first/2u&&last<first*2u);
    rev_frz=0;for(int b=0;b<6;b++)fx_buses(zero,zero,zero,l,rr,CTL);
    assert(reverb_model.cur==0&&reverb_model.gain==32768);
    for(int i=0;i<CTL;i++)assert(l[i]==0&&rr[i]==0);
    puts("PASS SPRING: four size/damping corners, decaying impulse, 12-second bounded FREEZE, deferred model switch and cleared tails");
}
int main(void) { half_transport();double_transport(); half_clock_and_slicer(); combination_holds(); routed_fx_sources(); spring_reverb(); return 0; }
