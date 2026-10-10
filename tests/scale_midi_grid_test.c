/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
static int gated(uint32_t part,uint32_t pitch) {
    for(int i=0;i<NVOICE;i++)if(trk[part].v[i].gate&&trk[part].v[i].note==pitch)return 1;
    return 0;
}
static void midi(uint32_t source,uint32_t ch,uint32_t note,uint32_t vel) {
    midi_in_q[mi_w++%MQ]=((source?0x10u:0u)|(vel?0x9u:0x8u))|(((vel?0x90u:0x80u)+ch)<<8)|(note<<16)|(vel<<24);
    frame();
}
static uint32_t owners(void) { uint32_t n=0;for(int i=0;i<MIDI_SCALE_HELD;i++)n+=midi_scale_held[i].part!=0;return n; }
int main(void) {
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(4);song.sel=1;go_home();frame();
    track_t *t=&trk[1];t->p[P_VOICE]=V_POLY;
    for(int root=0;root<12;root++)for(int scale=0;scale<NSCALES;scale++)for(int mode=0;mode<4;mode++) {
        t->p[P_ROOT]=root;t->p[P_SCALE]=scale;t->p[P_QUANT]=mode;
        for(int k=0;k<27;k++)assert(kb_map(t,k)==kb_scale_map(t,53+k));
        for(int note=0;note<128;note++){ uint32_t mapped=kb_scale_map(t,note);assert(mapped==KB_SILENT||mapped<128); }
    }
    t->p[P_ROOT]=0;t->p[P_SCALE]=2;t->p[P_QUANT]=2;song.octave=0;t->p[P_TRANS]=0;
    assert(kb_map(t,64-53)==63);
    char name[12];scale_note_name(name,t,63);assert(!strcmp(name,"Eb4"));
    scale_pitch_name(name,t,68);assert(!strcmp(name,"Ab"));scale_pitch_name(name,t,70);assert(!strcmp(name,"Bb"));
    t->p[P_ROOT]=6;t->p[P_SCALE]=1;scale_pitch_name(name,t,65);assert(!strcmp(name,"E#"));
    t->p[P_ROOT]=1;scale_note_name(name,t,72);assert(!strcmp(name,"C5"));
    t->p[P_ROOT]=0;t->p[P_SCALE]=2;
    midi_scale=0;midi(0,1,64,100);assert(gated(1,64));
    midi_scale=1;midi(0,1,64,0);assert(!gated(1,64)&&!owners()); /* mode changed mid-note */
    midi(0,1,64,100);assert(gated(1,63)&&!gated(1,64));
    t->p[P_ROOT]=2;t->p[P_SCALE]=1;song.octave=1;t->p[P_TRANS]=5;midi_scale=0;
    midi(0,1,64,0);assert(!gated(1,63)&&!owners()); /* original sounded pitch */
    t->p[P_ROOT]=0;t->p[P_SCALE]=2;song.octave=0;t->p[P_TRANS]=0;midi_scale=1;
    midi(0,1,61,100);assert(owners()==1);midi_scale=0;midi(0,1,61,0);assert(!owners()); /* WHITE black key ignored */
    midi_scale=1;t->p[P_QUANT]=1;
    midi(0,1,64,100);midi(0,1,63,100);assert(gated(1,63));
    midi(0,1,64,0);assert(gated(1,63));midi(0,1,63,0);assert(!gated(1,63)); /* SNAP collision */
    midi(0,1,64,100);midi(1,1,64,100);assert(owners()==2);
    midi(0,1,64,0);assert(gated(1,63)&&owners()==1&&midi_sel_on[1][64]);
    midi(0,1,64,100);assert(owners()==2);
    usb.resets++;frame();assert(gated(1,63)&&owners()==1);midi(1,1,64,0);assert(!gated(1,63)&&!owners());
    t->p[P_QUANT]=2;song.sel=1;midi(0,4,64,100);song.sel=2;midi(0,4,64,0);assert(!gated(1,63)&&!owners());
    midi(0,1,64,100);panic_req=2;frame();assert(!owners()&&!gated(1,63));midi(0,1,64,0);assert(!owners());
    /* Capacity rejection must not evict a held owner or release an unrelated pitch. */
    t->p[P_QUANT]=0;for(int n=0;n<MIDI_SCALE_HELD;n++)midi(0,1,n,100);assert(owners()==MIDI_SCALE_HELD);
    midi(0,1,100,100);assert(owners()==MIDI_SCALE_HELD&&!midi_sel_on[1][100]);
    midi_scale=0;midi(0,1,100,0);assert(owners()==MIDI_SCALE_HELD);
    for(int n=0;n<MIDI_SCALE_HELD;n++)midi(0,1,n,0);assert(!owners());
    ui.menu=1;ui.menu_sel=MI_MIDISCALE;mi_set(MI_MIDISCALE,1);assert(midi_scale);
    uint32_t word=lights_word();midi_scale=0;lights_from_word(word);assert(midi_scale);
    lights_from_word(word&~(1u<<23));assert(!midi_scale);ui.menu=0;
    outdir="build/host";song.sel=1;t->p[P_ROOT]=0;t->p[P_SCALE]=2;t->p[P_QUANT]=2;
    go_home();frame();press(B_SCL);frames(12);ppm("scale-grid-c-minor");release(B_SCL);
    live_mod[0].sync=5;press(B_LFO);frames(12);ppm("vibrato-scale-grid");release(B_LFO);
    live_mod[1].sync=5;press(B_ENV);frames(12);ppm("tremolo-scale-grid");release(B_ENV);
    puts("scale/MIDI grids: spelling, all roots/scales, held pitch ownership, collisions, USB/TRS, panic, capacity and settings PASS");
    return 0;
}
