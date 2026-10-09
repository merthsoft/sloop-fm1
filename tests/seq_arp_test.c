/* Production sequencer/arp ownership slice. Assertions remain enabled at -O2. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? i : 0; }
#include "../firmware/src/project.c"

static int sounding[128];
static unsigned attacks, releases;
static void drain(void)
{
    while (mo_r != mo_w) {
        uint32_t p = midi_out_q[mo_r++ % MOUT_Q], st = (p >> 8) & 0xf0, n = (p >> 16) & 127;
        if (st == 0x90) { assert(!sounding[n]); sounding[n] = 1; attacks++; }
        if (st == 0x80) { assert(sounding[n]); sounding[n] = 0; releases++; }
    }
}
static void reset(void)
{
    seq_stop(); host_tracks_init();
    memset(seq_harmony, 0, sizeof seq_harmony);
    memset(sounding, 0, sizeof sounding);
    memset(mo_set, 0, sizeof mo_set);
    mo_r = mo_w = 0; attacks = releases = 0;
    song.playing = 0; song.rec = 0; song.g[G_MIDI] = 1; song.g[G_BPM] = 120;
    usb.config = 1; fm1_in.notes = fm1_in.buttons = kb_prev = 0;
    trk[0].p[P_AMODE] = ARP_PULSE; trk[0].p[P_AORDER] = AORDER_SEQ_NOTE;
    trk[0].p[P_AOCT] = 1;
    events_block(CTL); /* settle the production route/mode transition first */
}
static step_t chord(uint8_t a, uint8_t b, uint8_t c)
{ step_t s = {0}; s.n = 3; s.note[0] = a; s.note[1] = b; s.note[2] = c; return s; }
static int pool_has(uint8_t n)
{ uint8_t p[64]; uint32_t count = arp_list(&trk[0], p); for (uint32_t i=0;i<count;i++) if(p[i]==n)return 1; return 0; }
int main(void)
{
    step_t a = chord(60,64,67), b = chord(60,65,69), rest = {0}, tie = {0};
    rest.time = ST_REST; tie.time = ST_TIE;
    reset();
    /* Normal production events path: recorded chord feeds PULSE, no direct duplicate.
     * Recording replay suppression and ratchets do not suppress/duplicate the source. */
    a.rat = 0xff; trk[0].step[0] = a; trk[0].rskip_n = 3;
    memcpy(trk[0].rskip,a.note,3); trk[0].rskip_abs = 0;
    seq_start(); events_block(CTL); drain();
    assert(seq_harmony[0].n == 3 && trk[0].seq_n == 0 && trk[0].arp_n == 3 && attacks == 3);
    seq_step(&trk[0], &tie, BEAT_U/4, 0); assert(seq_harmony[0].n == 3);
    input_on(&trk[0],60,100); input_on(&trk[0],72,100);
    seq_step(&trk[0], &b, BEAT_U/4, 0); drain();
    assert(pool_has(60) && pool_has(72) && pool_has(65) && !pool_has(64));
    input_off(&trk[0],60); assert(pool_has(60)); /* sequence still owns it */
    seq_step(&trk[0], &rest, BEAT_U/4, 0);
    assert(!pool_has(60) && pool_has(72) && seq_harmony[0].n == 0);
    seq_stop(); drain(); assert(attacks == releases);

    reset(); trk[0].p[P_AHOLD] = 1;
    input_on(&trk[0],72,100); input_off(&trk[0],72);
    seq_step(&trk[0], &a, BEAT_U/4, 0); seq_step(&trk[0], &rest, BEAT_U/4, 0);
    assert(pool_has(72) && !pool_has(60)); /* HOLD never retains a sequence rest */
    seq_step(&trk[0], &a, BEAT_U/4, 0);
    panic_req = 1; events_block(CTL); assert(!seq_harmony[0].n && !trk[0].nheld); drain();
    seq_step(&trk[0], &a, BEAT_U/4, 0);
    project_t saved; proj_capture(&saved); proj_apply(&saved,1);
    assert(!seq_harmony[0].n && trk[0].p[P_AORDER] == AORDER_SEQ_NOTE);

    reset(); trk[0].step[0] = a; seq_start(); events_block(CTL); drain();
    trk[0].p[P_AORDER] = AORDER_NOTE; events_block(CTL); drain();
    assert(!seq_harmony[0].n && trk[0].seq_n == 3); /* legacy direct playback returns */
    seq_stop(); drain(); assert(attacks == releases);
    reset(); trk[0].p[P_AMODE] = 0; seq_step(&trk[0],&a,BEAT_U/4,0);
    assert(trk[0].seq_n == 3 && !seq_harmony[0].n); seq_stop(); drain();

    reset(); trk[0].step[0] = a; trk[0].fill[0] = 1; /* FILL ONLY rejected */
    seq_start(); events_block(CTL); assert(!seq_harmony[0].n && !trk[0].arp_n);
    /* Explicit active count permits pitch zero; shared pitch membership deduplicates. */
    a = chord(0,0,127); seq_step(&trk[0],&a,BEAT_U/4,0);
    assert(seq_harmony[0].n == 2 && pool_has(0) && pool_has(127));
    reset(); trk[0].p[P_AMODE] = ARP_UP;
    a = chord(60,64,67); seq_step(&trk[0],&a,BEAT_U/4,7);
    assert(seq_harmony[0].n == 3); /* recording skip is deliberately ignored */
    /* Actual USB/TRS decoder boundary: live release cannot erase sequence pitches. */
    midi_in_q[mi_w++ % MQ] = 0x09u | 0x90u << 8 | 60u << 16 | 100u << 24;
    events_block(CTL); assert(trk[0].nheld == 1 && pool_has(60));
    midi_in_q[mi_w++ % MQ] = 0x08u | 0x80u << 8 | 60u << 16;
    events_block(CTL); assert(!trk[0].nheld && pool_has(60));
    assert(trk[0].arp_n == 1 && trk[0].arp_notes[0] == 60);
    unsigned gates = 0;
    for (uint32_t i=0;i<NVOICE;i++) gates += trk[0].v[i].gate != 0 && trk[0].v[i].note == 60;
    assert(gates); /* source release cannot prematurely end the audio arp gate */
    midi_in_q[mi_w++ % MQ] = 0x19u | 0x90u << 8 | 72u << 16 | 100u << 24;
    events_block(CTL); seq_step(&trk[0],&rest,BEAT_U/4,0);
    assert(pool_has(72) && !pool_has(60));
    midi_in_q[mi_w++ % MQ] = 0x18u | 0x80u << 8 | 72u << 16;
    events_block(CTL); assert(!pool_has(72)); seq_stop(); drain(); assert(attacks == releases);

    reset(); song.sel = song.octave = 0; trk[0].p[P_AHOLD] = 1;
    trk[0].p[P_CHORD] = CH_TRIAD; trk[0].p[P_SCALE] = 1;
    fm1_in.notes = 1u << 7; keyboard_block(); fm1_in.notes = 0; keyboard_block();
    assert(arp_chord_latch_n[0] == 3 && trk[0].held[1] == 64);
    seq_step(&trk[0],&b,BEAT_U/4,0);
    fm1_in.notes = 1u << 1; keyboard_block(); fm1_in.notes = 0; keyboard_block();
    assert(arp_chord_latch_notes[0][1] == 63 && pool_has(63) && pool_has(65));
    seq_step(&trk[0],&rest,BEAT_U/4,0); assert(pool_has(63) && !pool_has(65));
    seq_stop(); assert(!trk[0].nheld && !seq_harmony[0].n && !arp_chord_latch_n[0]);

    /* Format and enum migration: old IDs/defaults stay live-only; new routing
     * round-trips in the unchanged format. No transient ownership is persisted. */
    reset(); assert(TP[P_AORDER].def == AORDER_NOTE && TP[P_AORDER].max == AORDER_SEQ_PLAY);
    assert(sizeof(step_t) == 10 && sizeof(harmony_seq_source) == 6);
    proj_capture(&saved); assert(proj_ok(&saved));
    trk[0].p[P_AORDER] = AORDER_NOTE; proj_apply(&saved,1);
    assert(trk[0].p[P_AORDER] == AORDER_SEQ_NOTE && !seq_harmony[0].n);
    reset(); trk[0].step[0] = a; trk[0].step[1] = tie; trk[0].step[2] = rest;
    trk[0].micro[0] = 16;
    uint32_t u = div_units(trk_div(&trk[0]));
    seq_start(); seq_tick(&trk[0],1); assert(!seq_harmony[0].n);
    clk_pos = u/4; seq_tick(&trk[0],1); assert(seq_harmony[0].n == 3);
    clk_pos = u; seq_tick(&trk[0],1); assert(seq_harmony[0].n == 3);
    clk_pos = 2*u; seq_tick(&trk[0],1); assert(!seq_harmony[0].n);
    seq_stop();
    reset(); for(uint32_t i=0;i<16;i++) input_on(&trk[0],32+i,100);
    seq_step(&trk[0],&a,u,0); uint8_t pool[64]; trk[0].p[P_AOCT] = 4;
    assert(arp_list(&trk[0],pool) == 64 && trk[0].nheld == 16 && seq_harmony[0].n == 3);
    steps_clear(&trk[0]); assert(!seq_harmony[0].n);
    printf("seq arp: production boundaries, direct suppression, ownership, HOLD, ties/rests, fill, route, panic, project and MIDI balance passed; state=%zu bytes\n",sizeof seq_harmony);
    return 0;
}
