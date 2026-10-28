/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

static void setup(void)
{
    host_tracks_init();song.sel=0;song.playing=song.rec=transport_req=rec_wait=ft_on=0;
    track_t *t=&trk[0];kh_release(t);t->seq_n=0;
    t->p[P_AMODE]=t->p[P_CHORD]=t->p[P_AHOLD]=0;
    t->p[P_QUANT]=0;t->p[P_ROOT]=0;t->p[P_SCALE]=0;t->p[P_VOICE]=V_POLY;
    evolve_p[0][0]=DRONE_MOVE;evolve_p[0][1]=2;evolve_p[0][2]=101;
    evolve_p[0][3]=1;evolve_p[0][4]=evolve_p[0][5]=4;
    memset(evolve,0,sizeof evolve);evolve[0].rate=2;evolve[0].running=1;
}
static void cycle(void)
{
    int32_t expected=evolve[0].rng?evolve[0].rng:(int32_t)0x45564f4cu;
    noise32(&expected);uint32_t global=rng_state;
    evolve[0].left=1;evolve_tick(1);
    assert(evolve[0].rng==expected && rng_state==global);
}
static unsigned difference(const uint32_t *before)
{
    unsigned diff=0;for(unsigned n=0;n<128;n++)diff+=((before[n>>5]>>(n&31))&1u)!=(unsigned)kh_has(&trk[0],n);
    return diff;
}
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";panel=PANEL_DEFAULT;layers_init();palette_set(NPALETTES-1u);
    setup();track_t *t=&trk[0];
    /* A nearly full octave leaves just one destination. The random choice will
     * usually be blocked; the exhaustive fallback must find its neighbour. */
    for(unsigned n=60;n<72;n++)if(n!=65)drone_toggle(t,n,27);
    for(unsigned i=0;i<1000;i++) {
        uint32_t before[4];memcpy(before,kh_set[0],sizeof before);cycle();
        assert(difference(before)==2);
        unsigned count=0;for(unsigned n=0;n<128;n++)if(kh_has(t,n)){assert(n>=60&&n<72);count++;}
        assert(count==11);
    }
    /* Fully occupied bounds: no legal move exists, so preserve every owner and
     * voice and terminate. No speculative releases or new random draws. */
    for(unsigned n=60;n<72;n++)if(!kh_has(t,n))drone_toggle(t,n,27);
    uint32_t before[4];memcpy(before,kh_set[0],sizeof before);
    voice_t voices[NVOICE];memcpy(voices,t->v,sizeof voices);uint32_t midi=mo_w;
    cycle();assert(!difference(before) && !memcmp(voices,t->v,sizeof voices) && mo_w==midi);
    /* Range is a real constraint even when the octave contains empty notes. */
    setup();drone_toggle(t,60,27);t->seq_n=1;t->seq_notes[0]=61;
    memcpy(before,kh_set[0],sizeof before);cycle();assert(!difference(before));
    evolve_p[0][3]=2;cycle();assert(kh_has(t,62) && !kh_has(t,60));
    /* Source and destination ownership are both protected. A different drone
     * source may still move, even if the first choice belongs to the sequence. */
    setup();drone_toggle(t,60,27);drone_toggle(t,64,27);drone_toggle(t,67,27);
    t->seq_n=3;t->seq_notes[0]=60;t->seq_notes[1]=63;t->seq_notes[2]=65;
    cycle();assert(kh_has(t,60)&&kh_has(t,64)&&!kh_has(t,67));
    assert(kh_has(t,66)||kh_has(t,68));
    /* The exhaustive fallback cannot bypass actual physical or MIDI owners. */
    setup();drone_toggle(t,60,27);memcpy(before,kh_set[0],sizeof before);
    kb_kind[0]=KS_NOTE;kb_n[0]=1;kb_nt[0][0]=60;kb_trk[0]=0;
    cycle();assert(!difference(before));kb_kind[0]=KS_NONE;kb_n[0]=0;
    midi_scale_input(t,60,60,100,1);cycle();assert(!difference(before));
    midi_scale_input(t,60,60,0,0);
    midi_scale_input(t,61,61,100,1);cycle();assert(!difference(before));
    midi_scale_input(t,61,61,0,0);cycle();assert(kh_has(t,61)&&!kh_has(t,60));
    /* Ordinary 100% keeps single-attempt semantics and can still skip. */
    setup();evolve_p[0][2]=100;drone_toggle(t,60,27);
    t->seq_n=1;t->seq_notes[0]=61;memcpy(before,kh_set[0],sizeof before);
    for(unsigned i=0;i<100;i++){cycle();assert(!difference(before));}
    /* Native CHANCE actually displays and reaches the requested extra setting. */
    setup();open_family(FAM_LFO);
    for(unsigned p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_EVOLVE&&PAGES[p].id[0]==0)ui.page=p;
    page_entered();edit_param(2,1000);assert(evolve_p[0][2]==101);
    ui.msg_t=0;ui.force=1;ui_draw();assert(!strcmp(ui.big_v[2],"101")&&!strcmp(ui.big_u[2],"%"));
    ppm("drone-force");
    puts("101% evolution: forced valid changes, bounded no-move case, range/ownership, single RNG draw, legacy 100% and native display PASS");
    return 0;
}
