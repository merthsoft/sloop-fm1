/* Independent ties through the production sequencer, voices, MIDI and projects. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? i : 0; }
#include "../firmware/src/project.c"

static unsigned ons[128], offs[128], active[128];
static void drain(void)
{
    while (mo_r != mo_w) {
        uint32_t p = midi_out_q[mo_r++ % MOUT_Q], status = (p >> 8) & 0xf0u, n = (p >> 16) & 127u;
        if (status == 0x90u) { assert(!active[n]); active[n] = 1; ons[n]++; }
        if (status == 0x80u) { assert(active[n]); active[n] = 0; offs[n]++; }
    }
}
static void block(void)
{
    int32_t out[CTL * 2]; mix_block(out, CTL); drain();
}
static void reset(void)
{
    seq_stop(); drain(); host_tracks_init();
    memset(ons, 0, sizeof ons); memset(offs, 0, sizeof offs); memset(active, 0, sizeof active);
    memset(mo_set, 0, sizeof mo_set); mo_r = mo_w = 0;
    song.playing = song.rec = rec_wait = 0; transport_req = 0;
    song.g[G_MIDI] = 1; song.g[G_BPM] = 120; usb.config = 1;
    fm1_in.notes = fm1_in.buttons = kb_prev = 0;
    for (unsigned i = 0; i < NTRK; i++) steps_clear(&trk[i]);
    track_t *t = &trk[0];
    t->p[P_VOICE] = V_POLY; t->p[P_AMODE] = 0; t->p[P_CHORD] = 0;
    t->p[P_STRUM] = 0; t->p[P_SLEN] = 8; t->p[P_SDIV] = 2; t->p[P_SGATE] = 16;
}
static void until(unsigned idx)
{
    unsigned limit = FS * 4 / CTL;
    do { block(); assert(limit--); } while (trk[0].seq_idx != idx);
}
static voice_t *voice(unsigned note)
{
    for (unsigned i = 0; i < NVOICE; i++)
        if (trk[0].v[i].gate && trk[0].v[i].note == note) return &trk[0].v[i];
    assert(0); return 0;
}
static void stopped(void)
{
    seq_stop(); drain();
    for (unsigned i = 0; i < 128; i++) assert(!active[i] && ons[i] == offs[i]);
    assert(!trk[0].seq_n);
}
int main(void)
{
    reset(); track_t *t = &trk[0];
    t->step[0] = (step_t){.note={60}, .n=1, .vel=82};
    t->step[1] = (step_t){.note={60,64}, .n=2, .vel=100, .flags=SF_TIE(0)};
    t->step[2] = (step_t){.note={60,64,67}, .n=3, .vel=110, .flags=SF_TIE(0)|SF_TIE(1)};
    t->step[3] = (step_t){.note={64,67}, .n=2, .flags=SF_TIE(0)|SF_TIE(1)};
    t->step[4] = (step_t){.note={67,60}, .n=2, .vel=90, .flags=SF_TIE(0)};
    seq_start(); until(0);
    voice_t *c = voice(60); uint32_t c_age = c->age;
    until(1); assert(ons[60]==1 && !offs[60] && ons[64]==1);
    assert(voice(60)==c && c->age==c_age && c->ph[0]);
    voice_t *e = voice(64); uint32_t e_age = e->age;
    until(2); assert(ons[60]==1 && ons[64]==1 && ons[67]==1);
    assert(voice(64)==e && e->age==e_age);
    until(3); assert(offs[60]==1 && !offs[64] && !offs[67]);
    until(4); assert(offs[64]==1 && ons[60]==2 && ons[67]==1);
    until(5); assert(!t->seq_n && offs[67]==1); stopped();

    /* Gate expiry compacts slots without losing the held pitch's velocity/mask. */
    reset();
    t->step[0] = (step_t){.note={64,60},.n=2,.vel=83};
    t->step[1] = (step_t){.note={67,60},.n=2,.vel=120,.flags=SF_TIE(1)};
    t->step[2] = (step_t){.note={60},.n=1,.flags=SF_TIE(0)};
    seq_start(); until(0);
    unsigned limit = FS/CTL;
    while (!offs[64]) { block(); assert(limit--); }
    assert(t->seq_n==1 && t->seq_notes[0]==60 && t->seq_hold==SF_TIE(0));
    uint8_t held_velocity = seq_direct_vel[0][0];
    until(1); assert(ons[60]==1 && !offs[60] && seq_direct_vel[0][1]==held_velocity);
    until(2); assert(ons[60]==1 && !offs[60]); stopped();

    /* Incoming ties don't ratchet; the new note on the same step still does. */
    reset(); t->p[P_SGATE]=127;
    t->step[0]=(step_t){.note={60},.n=1,.vel=90};
    t->step[1]=(step_t){.note={60,64},.n=2,.vel=90,.flags=SF_TIE(0),.rat=0x0f};
    t->step[2]=(step_t){.note={60},.n=1,.flags=SF_TIE(0)};
    seq_start(); until(2); assert(ons[60]==1 && !offs[60] && ons[64]==4); stopped();

    /* A ratchet value attached only to a continued note cannot shorten the new note. */
    reset(); t->p[P_SGATE]=127;
    t->step[0]=(step_t){.note={60},.n=1,.vel=90};
    t->step[1]=(step_t){.note={60,64},.n=2,.vel=90,.flags=SF_TIE(0),.rat=3};
    t->step[2]=(step_t){.note={60},.n=1,.flags=SF_TIE(0)};
    seq_start(); until(1); assert(t->seq_off>BEAT_U/8);
    until(2); assert(ons[60]==1 && ons[64]==1); stopped();

    /* Starting on a continuation attacks once; wrap ties stay one voice. */
    reset(); t->p[P_SLEN]=2;
    t->step[0]=(step_t){.note={60,64},.n=2,.vel=80,.flags=SF_TIE(0)};
    t->step[1]=(step_t){.note={60},.n=1,.flags=SF_TIE(0)};
    seq_start(); until(1); until(0); until(1);
    assert(ons[60]==1 && !offs[60] && ons[64]==2); stopped();

    /* Four voices, reordered slots and a new attack don't change tie identity. */
    reset();
    t->step[0]=(step_t){.note={60,64,67,71},.n=4,.vel=90};
    t->step[1]=(step_t){.note={71,72,60,67},.n=4,.vel=110,
        .flags=SF_TIE(0)|SF_TIE(2)|SF_TIE(3)};
    seq_start(); until(1);
    assert(ons[60]==1 && ons[67]==1 && ons[71]==1 && ons[72]==1 && offs[64]==1);
    stopped();

    /* A skipped entry cannot become a silently owned continuation. */
    reset();
    t->step[0]=(step_t){.note={60},.n=1,.vel=90};
    t->step[1]=(step_t){.note={60,64},.n=2,.vel=90,.flags=SF_TIE(0)};
    seq_start(); until(0); t->seq_idx=1;
    seq_step(t,&t->step[1],BEAT_U/4,1); drain();
    assert(t->seq_n==1 && t->seq_notes[0]==64 && !active[60]); stopped();

    /* A failed fill condition still releases notes protected by lookahead. */
    reset();
    t->step[0]=(step_t){.note={60},.n=1,.vel=90};
    t->step[1]=(step_t){.note={60,64},.n=2,.vel=90,.flags=SF_TIE(0)};
    step_fill_set(t,1,FC_FILL); assert(!step_plays(t,1));
    seq_start(); until(1); assert(!t->seq_n && !active[60] && !ons[64]); stopped();

    /* Explicit retriggers and legacy all-note ties retain their original meaning. */
    reset(); t->p[P_SGATE]=127;
    t->step[0]=(step_t){.note={60},.n=1,.vel=90};
    t->step[1]=(step_t){.time=ST_TIE};
    t->step[2]=(step_t){.note={60},.n=1,.vel=90};
    seq_start(); until(1); assert(ons[60]==1 && !offs[60]);
    until(2); assert(ons[60]==2 && offs[60]==1); stopped();

    /* The existing ten-byte saved step preserves the complete continuation mask. */
    reset();
    t->step[0]=(step_t){.note={60,64,67,71},.n=4,.vel=99,
        .flags=SF_ACCENT|SF_SLIDE|SF_TIE(0)|SF_TIE(2)|SF_TIE(3),.lvl=0x39,.rat=0x24};
    step_t saved_step=t->step[0]; project_t saved;
    proj_capture(&saved); assert(proj_ok(&saved));
    steps_clear(t); proj_apply(&saved,1);
    assert(sizeof(step_t)==10 && !memcmp(&saved_step,&t->step[0],sizeof saved_step));
    assert(saved_step.flags<128); /* all four ties fit the existing 7-bit flags field */
    puts("per-note ties: overlapping voices, gates, ratchets, loop, MIDI and project round trip PASS");
    return 0;
}
