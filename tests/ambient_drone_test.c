/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static int gates(track_t *t,uint32_t note)
{
    for(unsigned i=0;i<NVOICE;i++)if(t->v[i].active && t->v[i].gate && t->v[i].note==note)return 1;
    return 0;
}
static int balance[16][128];
static void drain(void)
{
    while(mo_r!=mo_w) {
        uint32_t p=midi_out_q[mo_r++%MOUT_Q],s=(p>>8)&255u,n=(p>>16)&127u,v=p>>24;
        if((s&240u)==144u && v)balance[s&15u][n]++;
        if((s&240u)==128u || ((s&240u)==144u && !v))balance[s&15u][n]--;
        assert(balance[s&15u][n]>=0 && balance[s&15u][n]<=1);
    }
}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    song.sel=0;go_home();track_t *t=&trk[0];
    t->p[P_AMODE]=t->p[P_CHORD]=t->p[P_AHOLD]=0;t->p[P_SUS]=127;
    song.g[G_CH1]=1;song.g[G_CH2]=2;
    transport_req=1;frame();assert(song.playing);
    step_t original[NSTEP];memcpy(original,t->step,sizeof original);
    t->p[P_AMODE]=t->p[P_CHORD]=1; /* selecting DRONE takes over without a stop/disarm ritual */
    evolve_p[0][0]=DRONE_STILL;frame();assert(drone_on(t) && !t->p[P_AMODE] && !t->p[P_CHORD]);
    assert(song.playing && !memcmp(original,t->step,sizeof original));
    uint32_t n=kb_map(t,0),other=kb_map(t,4);
    key(0);drain();assert(kh_has(t,n) && gates(t,n));
    lights_notes=0;assert(keys_lit()&1u);
    key(4);drain();assert(kh_has(t,n) && kh_has(t,other) && gates(t,n));
    key(4);drain();assert(!kh_has(t,other) && kh_has(t,n) && !gates(t,other));
    /* Octave remapping never changes the captured pitch; return and tap removes it. */
    int8_t oct=song.octave;song.octave++;frame();assert(kh_has(t,n) && gates(t,n));
    song.octave=oct;frame();key(0);drain();assert(!kh_has(t,n) && !gates(t,n));
    /* Incoming MIDI shares a drone without retrigger or either owner cutting the other. */
    key(0);drain();midi_scale_input(t,60,n,100,1);
    midi_scale_input(t,60,n,0,0);assert(gates(t,n) && kh_has(t,n));
    midi_scale_input(t,60,n,100,1);key(0);drain();assert(!kh_has(t,n) && gates(t,n));
    midi_scale_input(t,60,n,0,0);assert(!gates(t,n));
    /* Sequencer gates cannot cut or reattack a shared drone pitch. Removing
     * the drone leaves an existing sequence owner until its own gate ends. */
    key(0);drain();assert(gates(t,n));
    t->seq_n=1;t->seq_notes[0]=(uint8_t)n;
    voice_t before[NVOICE];memcpy(before,t->v,sizeof before);
    trk_note_on(t,n,70);assert(!memcmp(before,t->v,sizeof before));
    trk_note_off(t,n);assert(gates(t,n));
    key(0);drain();assert(gates(t,n) && !kh_has(t,n));
    seq_release(t);assert(!gates(t,n));
    /* Track changes preserve captured channel and pitch ownership. */
    key(0);drain();track_select(1);frame();assert(kh_has(t,n) && gates(t,n));
    track_select(0);frame();
    evolve_p[0][0]=DRONE_OFF;frame();drain();assert(!kh_any(t) && !gates(t,n));
    /* ARP/chord takeover, route changes and STOP retire the drone owners. */
    for(unsigned reason=0;reason<5;reason++) {
        t->p[P_AMODE]=t->p[P_CHORD]=0;evolve_p[0][0]=DRONE_STILL;frame();key(0);drain();
        assert(kh_has(t,n));
        if(reason==0)t->p[P_AMODE]=1;
        if(reason==1)t->p[P_CHORD]=1;
        if(reason==2)song.g[G_CH1]=2;
        if(reason==3)transport_req=2;
        if(reason==4)panic_req=1; /* same retirement as preset/engine/project adoption */
        frame();drain();assert(!kh_any(t));
        t->p[P_AMODE]=t->p[P_CHORD]=0;
    }
    for(unsigned c=0;c<16;c++)for(unsigned p=0;p<128;p++)assert(!balance[c][p]);
    /* Ordinary HOLD preference does not turn tap-add into replacement behavior. */
    t->p[P_AHOLD]=1;evolve_p[0][0]=DRONE_STILL;frame();key(0);key(4);drain();
    assert(kh_has(t,n) && kh_has(t,other));evolve_p[0][0]=DRONE_OFF;frame();drain();
    assert(!kh_any(t));t->p[P_AHOLD]=0;
    for(unsigned c=0;c<16;c++)for(unsigned p=0;p<128;p++)assert(!balance[c][p]);
    /* Capture the native UI without a hardware phone or device. */
    open_family(FAM_LFO);for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_EVOLVE && PAGES[p].id[0]==0)ui.page=p;
    evolve_p[0][0]=DRONE_STILL;page_entered();frame();key(0);key(4);ui.msg_t=0;ui.force=1;ui_draw();ppm("ambient-drone");
    /* DRIFT on/off is independent of latching and moving/still selection. */
    drift_p[0][0]=1;frame();assert(drone_on(t) && kh_has(t,n));
    drift_p[0][0]=0;frame();assert(drone_on(t) && kh_has(t,n));
    evolve_p[0][0]=DRONE_MOVE;frame();assert(kh_has(t,n));
    evolve_p[0][0]=DRONE_STILL;frame();assert(kh_has(t,n));
    evolve_p[0][0]=DRONE_OFF;frame();drain();
    /* DRONE-only must not advance DRIFT or consume its RNG. */
    memset(drift,0,sizeof drift);evolve_p[0][0]=DRONE_STILL;frame();
    assert(!drift[0].phase && !drift[0].value);
    puts("drone: tap/add/remove, octave/track snapshots, MIDI overlap/channel balance, takeover and STOP PASS");
    return 0;
}
