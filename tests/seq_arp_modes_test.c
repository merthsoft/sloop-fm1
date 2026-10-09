/* SPDX-License-Identifier: GPL-3.0-only */
/* Runtime mode changes must affect the real MIDI output of a sequencer-fed
 * chord held across fifteen ties, without replaying a grid slot. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static uint8_t note_trace[512], velocity_trace[512];
static uint32_t grid_trace[512], ntrace, current_grid;
static int active[128];
static unsigned note_offs;
static void capture(void)
{
    while (mo_r != mo_w) {
        uint32_t p=midi_out_q[mo_r++ % MOUT_Q], st=(p>>8)&0xf0, note=(p>>16)&127;
        if(st==0x90) {
            assert(!active[note]); active[note]=1;
            assert(ntrace<sizeof note_trace); note_trace[ntrace]=note;
            velocity_trace[ntrace]=(p>>24)&127; grid_trace[ntrace++]=current_grid;
        } else if(st==0x80) {
            assert(active[note]); active[note]=0; note_offs++;
        }
    }
}
static void at(uint32_t position)
{
    clk_beat=position/BEAT_U; clk_pos=position%BEAT_U;
    events_block(0); capture();
}
static void setup(uint32_t mode)
{
    seq_stop(); host_tracks_init();
    for(uint32_t i=0;i<NTRK;i++) steps_clear(&trk[i]);
    memset(seq_harmony,0,sizeof seq_harmony);
    memset(active,0,sizeof active); memset(mo_set,0,sizeof mo_set);
    mo_r=mo_w=mi_r=mi_w=0; ntrace=note_offs=0;
    song.playing=song.rec=0; song.g[G_MIDI]=1; song.g[G_BPM]=120;
    usb.config=1; fm1_in.notes=fm1_in.buttons=kb_prev=0;
    track_t *t=&trk[0];
    t->p[P_AMODE]=mode; t->p[P_AORDER]=AORDER_SEQ_NOTE;
    t->p[P_AOCT]=1; t->p[P_ARATE]=t->p[P_SDIV]=2;
    t->p[P_ASWING]=t->p[P_SSWING]=0;
    t->p[P_APROB]=127; t->p[P_SLEN]=16;
    t->step[0].time=0; t->step[0].n=4; t->step[0].note[0]=67;
    t->step[0].note[1]=60; t->step[0].note[2]=64; t->step[0].note[3]=69;
    for(uint32_t i=1;i<16;i++) t->step[i].time=ST_TIE;
    events_block(0); capture(); assert(!ntrace);
    seq_start();
}
static void constant_mode(uint32_t mode,const uint8_t *expected,uint32_t cycle)
{
    setup(mode); uint32_t u=div_units(trk_div(&trk[0]));
    for(uint32_t i=0;i<16;i++) {
        current_grid=i; at(i*u);
        assert(seq_harmony[0].n==4 && trk[0].seq_n==0);
        assert(ntrace==i+1 && note_trace[i]==expected[i%cycle]);
        assert(grid_trace[i]==i);
        at(i*u+u/2); assert(ntrace==i+1); /* no off-grid replay */
    }
    seq_stop();capture();assert(note_offs==ntrace);
}
static void route_transitions(void)
{
    setup(ARP_UP);track_t *t=&trk[0];uint32_t u=div_units(trk_div(t));
    t->step[0].vel=72;t->step[0].lvl=LV_NORM|(LV_GHOST<<2)|(LV_SOFT<<4)|(LV_HARD<<6);
    for(uint32_t i=0;i<5;i++) {current_grid=i;at(i*u);}
    uint32_t before=ntrace;
    t->p[P_AMODE]=ARP_OFF;at(4*u+u/4);
    assert(!seq_harmony[0].n&&t->seq_n==4&&ntrace==before+4);
    for(uint32_t i=0;i<4;i++) {
        assert(note_trace[before+i]==t->step[0].note[i]);
        assert(velocity_trace[before+i]==step_vel(&t->step[0],i));
    }
    t->p[P_AMODE]=ARP_UP;at(4*u+u/2);
    assert(seq_harmony[0].n==4&&!t->seq_n);
    seq_stop();capture();assert(note_offs==ntrace);
    /* Duplicate pitches are deduplicated by SNOTE. Their packed per-note
     * dynamics must be remapped to the surviving pitch order on return. */
    setup(ARP_UP);t->step[0].note[0]=60;t->step[0].note[1]=60;
    t->step[0].note[2]=64;t->step[0].note[3]=67;
    t->step[0].vel=72;t->step[0].lvl=LV_NORM|(LV_GHOST<<2)|(LV_SOFT<<4)|(LV_HARD<<6);
    at(0);at(u);assert(seq_harmony[0].n==3);
    before=ntrace;t->p[P_AMODE]=ARP_OFF;at(u+u/4);
    assert(t->seq_n==3&&ntrace==before+3);
    static const uint8_t original_indices[]={0,2,3};
    for(uint32_t i=0;i<3;i++) {
        assert(note_trace[before+i]==t->step[0].note[original_indices[i]]);
        assert(velocity_trace[before+i]==step_vel(&t->step[0],original_indices[i]));
    }
    seq_stop();capture();assert(note_offs==ntrace);
    /* An explicit rest followed by ties owns no source in either route. */
    setup(ARP_UP);t->step[1].time=ST_REST;
    at(0);at(u);assert(!seq_harmony[0].n&&!t->seq_n);
    before=ntrace;t->p[P_AMODE]=ARP_OFF;at(u+u/4);
    t->p[P_AMODE]=ARP_UP;at(u+u/2);at(2*u);
    assert(!seq_harmony[0].n&&!t->seq_n&&ntrace==before);
    seq_stop();capture();assert(note_offs==ntrace);
    /* A skipped NOTE clears the previous pool; scanning back from later TIEs
     * must never resurrect either old chord or the skipped replacement. */
    setup(ARP_UP);t->step[1]=t->step[0];t->step[1].note[0]=72;
    t->fill[0]=(uint8_t)(1u<<2); /* step2 FILL ONLY, no fill active */
    at(0);at(u);at(2*u);assert(!seq_harmony[0].n&&!t->seq_n);
    before=ntrace;t->p[P_AMODE]=ARP_OFF;at(2*u+u/4);
    t->p[P_AMODE]=ARP_UP;at(2*u+u/2);
    assert(!seq_harmony[0].n&&!t->seq_n&&ntrace==before);
    seq_stop();capture();assert(note_offs==ntrace);
    /* A skipped source at the loop start is equally silent when enabled. */
    setup(ARP_OFF);t->fill[0]=1;at(0);assert(!t->seq_n);
    t->p[P_AMODE]=ARP_UP;at(u/2);assert(!seq_harmony[0].n&&!ntrace);
    seq_stop();capture();assert(note_offs==ntrace);
}
int main(void)
{
    static const uint8_t up[]={60,64,67,69}, down[]={69,67,64,60}, updown[]={60,64,67,69,67,64};
    constant_mode(ARP_UP,up,4);constant_mode(ARP_DOWN,down,4);constant_mode(ARP_UPDOWN,updown,6);
    setup(ARP_UP);uint32_t u=div_units(trk_div(&trk[0]));
    static const uint8_t changed[]={60,64,67,69,60,69,67,64,60,69,60,64,67,69,67,64};
    for(uint32_t i=0;i<16;i++) {
        if(i==5||i==10) {
            trk[0].p[P_AMODE]=i==5?ARP_DOWN:ARP_UPDOWN;
            at((i-1)*u+3*u/4); /* change while the tied source is holding */
            assert(ntrace==i && seq_harmony[0].n==4);
        }
        current_grid=i;at(i*u);
        assert(ntrace==i+1 && note_trace[i]==changed[i] && grid_trace[i]==i);
        assert(seq_harmony[0].n==4 && trk[0].seq_n==0);
        at(i*u+u/2);assert(ntrace==i+1);
    }
    seq_stop();capture();assert(note_offs==ntrace);
    /* Change every single-note mode in one running song. All notes are traced
     * from the USB MIDI queue, not by directly calling arp_next. */
    setup(ARP_UP);uint32_t slot=0;
    for(uint32_t mode=ARP_UP;mode<ARP_PULSE;mode++) {
        if(slot) { trk[0].p[P_AMODE]=mode;at((slot-1)*u+3*u/4);assert(ntrace==slot); }
        printf("mode %u:",mode);
        for(uint32_t k=0;k<16;k++,slot++) {
            current_grid=slot;at(slot*u);assert(ntrace==slot+1);
            uint8_t note=note_trace[slot];
            assert(note==60||note==64||note==67||note==69);
            assert(grid_trace[slot]==slot&&seq_harmony[0].n==4);
            printf(" %u",note);at(slot*u+u/2);assert(ntrace==slot+1);
        }
        puts("");
        uint32_t start=slot-16;
        static const uint8_t order[]={67,60,64,69},outside[]={60,69,64,67},rootalt[]={60,64,60,67,60,69};
        static const uint8_t downup[]={69,67,64,60,64,67},repeat[]={60,64,67,69,69,67,64,60},inside[]={64,67,60,69};
        const uint8_t *cycle=0;uint32_t length=0;
        if(mode==ARP_UP) {cycle=up;length=4;}
        if(mode==ARP_DOWN) {cycle=down;length=4;}
        if(mode==ARP_UPDOWN) {cycle=updown;length=6;}
        if(mode==ARP_ORDER) {cycle=order;length=4;}
        if(mode==ARP_OUTSIDE) {cycle=outside;length=4;}
        if(mode==ARP_ROOTALT) {cycle=rootalt;length=6;}
        if(mode==ARP_DOWNUP) {cycle=downup;length=6;}
        if(mode==ARP_UPDOWN_REPEAT) {cycle=repeat;length=8;}
        if(mode==ARP_INSIDE) {cycle=inside;length=4;}
        for(uint32_t k=0;k<16;k++) if(cycle) assert(note_trace[start+k]==cycle[k%length]);
        if(mode==ARP_SHUFFLE) for(uint32_t k=0;k<16;k+=4)
            for(uint32_t a=0;a<4;a++) for(uint32_t b=a+1;b<4;b++) assert(note_trace[start+k+a]!=note_trace[start+k+b]);
        if(mode==ARP_WALK) for(uint32_t k=1;k<16;k++) {
            uint32_t a=0,b=0;
            while(up[a]!=note_trace[start+k-1])a++;
            while(up[b]!=note_trace[start+k])b++;
            assert(a+1==b||b+1==a);
        }
    }
    trk[0].p[P_AMODE]=ARP_PULSE;at((slot-1)*u+3*u/4);assert(ntrace==slot);
    for(uint32_t k=0;k<4;k++) {
        current_grid=slot+k;at((slot+k)*u);
        assert(ntrace==slot+4*(k+1));
        for(uint32_t j=0;j<4;j++) assert(note_trace[slot+4*k+j]==up[j]);
    }
    seq_stop();capture();assert(note_offs==ntrace);
    route_transitions();
    puts("seq arp modes: C/E/G/A +15ties emits distinct UP/DN/UPDN; runtime switches stay on grid, preserve source and balance MIDI PASS");
    return 0;
}
