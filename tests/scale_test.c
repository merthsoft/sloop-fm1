#undef NDEBUG /* Test assertions stay active in optimized host builds. */
/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the real keyboard, recording, arp and MIDI-out paths on the host.
 * Build with the same generated headers and flags as hostsim.c. */
#include <assert.h>
#define main hostsim_main
#include "hostsim.c"
#undef main

static const uint8_t WHITE_KEYS[] = {0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26};
static const struct { uint8_t count, notes[12]; } EXPECTED[] = {
    {12, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}},
    {7, {0, 2, 4, 5, 7, 9, 11}}, {7, {0, 2, 3, 5, 7, 8, 10}},
    {7, {0, 2, 3, 5, 7, 9, 10}}, {7, {0, 2, 4, 5, 7, 9, 10}},
    {5, {0, 2, 4, 7, 9}}, {5, {0, 3, 5, 7, 10}},
    {7, {0, 2, 3, 5, 7, 8, 11}}, {7, {0, 1, 3, 5, 7, 8, 10}},
    {7, {0, 2, 4, 6, 7, 9, 11}}, {7, {0, 1, 3, 5, 6, 8, 10}},
    {7, {0, 2, 3, 5, 7, 9, 11}}, {6, {0, 3, 5, 6, 7, 10}},
    {6, {0, 2, 4, 6, 8, 10}}, {8, {0, 1, 3, 4, 6, 7, 9, 10}},
    {8, {0, 2, 3, 5, 6, 8, 9, 11}},
};

static void mapping_test(void)
{
    track_t *t = &trk[0];
    uint32_t s, k, w;
    int root, oct, trans;
    assert(TP[P_SCALE].max + 1 == sizeof EXPECTED / sizeof EXPECTED[0]);
    assert(sizeof SCALE_MASK / sizeof SCALE_MASK[0] == sizeof EXPECTED / sizeof EXPECTED[0]);
    t->p[P_QUANT] = 2;
    for (s = 0; s <= (uint32_t)TP[P_SCALE].max; s++) {
        uint32_t mask = 0;
        t->p[P_SCALE] = (int16_t)s;
        for (k = 0; k < EXPECTED[s].count; k++)
            mask |= 1u << EXPECTED[s].notes[k];
        assert(scale_mask(t) == mask);
        for (root = 0; root < 12; root++)
            for (oct = -3; oct <= 3; oct++)
                for (trans = -24; trans <= 24; trans++) {
                    t->p[P_ROOT] = (int16_t)root;
                    song.octave = (int8_t)oct;
                    t->p[P_TRANS] = (int16_t)trans;
                    for (k = w = 0; k < 27u; k++) {
                        uint32_t actual = kb_map(t, k);
                        if (k == WHITE_KEYS[w]) {
                            /* Four white keys precede C4; use a positive cycle
                             * offset to independently handle the lower degrees. */
                            int d = (int)w - 4 + 12 * EXPECTED[s].count;
                            int want = 60 + root + 12 * (oct + d / EXPECTED[s].count - 12)
                                + EXPECTED[s].notes[d % EXPECTED[s].count] + trans;
                            assert(actual == (uint32_t)clamp(want, 0, 127));
                            w++;
                        } else {
                            assert(actual == KB_SILENT);
                        }
                    }
                }
    }
    t->p[P_QUANT] = 0;
    song.octave = 0;
    t->p[P_TRANS] = -5;
    for (k = 0; k < 27u; k++)
        assert(kb_map(t, k) == 48u + k);
    t->p[P_QUANT] = 2;
    for (k = 0; k < 27u; k++) {                /* the drum track: the key's lane, whatever the scale */
        TDRUM->p[P_QUANT] = 2;
        assert(kb_map(TDRUM, k) == LANE_NOTE[lane_of_key(k)]);
    }
    t->engine = t->eng_req = 4;
    if (drum_set() >= 0) {
        t->p[P_E0] = (int16_t)drum_set();
        for (k = 0; k < 27u; k++)
            assert(kb_map(t, k) == 36u + k);
    }
    t->engine = t->eng_req = 0;                /* SNAP (QNT 1, the old ON): every key, rounded down */
    t->p[P_QUANT] = 1;
    t->p[P_SCALE] = 2;                         /* C minor */
    t->p[P_ROOT] = 0;
    t->p[P_TRANS] = 0;
    song.octave = 0;
    assert(kb_map(t, 11) == 63u && kb_map(t, 10) == 63u && kb_map(t, 7) == 60u && kb_map(t, 8) == 60u);
    puts("scales: all 16 scales, 12 roots, octave/transpose ranges, bypass, drums and SNAP ok");
}

static void key_events_test(void)
{
    track_t *t = &trk[0];
    uint32_t before;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    host_tracks_init();
    kb_prev = 0;
    usb.config = 1;
    mo_w = mo_r = 0;
    t->p[P_QUANT] = 2;
    t->p[P_SCALE] = 2;                      /* C minor: E key plays Eb */
    t->p[P_AMODE] = 1;
    song.playing = song.rec = 1;
    fm1_in.notes = 1u << 8;                /* C# is silent */
    keyboard_block();
    assert(t->arp_phys == 0 && t->nheld == 0 && t->step[0].n == 0 && mo_w == 0);
    t->p[P_QUANT] = 0;                    /* releasing a muted key stays silent */
    fm1_in.notes = 0;
    keyboard_block();
    assert(mo_w == 0);
    t->p[P_QUANT] = 2;
    fm1_in.notes = (1u << 11) | (1u << 10); /* E and D#: only Eb sounds/records */
    keyboard_block();
    assert(t->arp_phys == 1 && t->nheld == 1 && t->held[0] == 63);
    assert(t->step[0].n == 0);             /* ARP on: what it plays is recorded, not the key */
    arp_tick(t, CTL * (uint32_t)song.g[G_BPM]);
    assert(t->step[0].n == 1 && t->step[0].note[0] == 63);
    assert(mo_w == 1 && ((midi_out_q[0] >> 16) & 127u) == 63);
    t->p[P_SCALE] = 9;
    t->p[P_ROOT] = 6;
    t->p[P_TRANS] = 12;
    song.octave = 1;
    song.sel = 1;                          /* key-up follows the original note/part */
    fm1_in.notes = 0;
    keyboard_block();
    assert(t->arp_phys == 0 && t->nheld == 0);
    assert(mo_w == 2 && ((midi_out_q[1] >> 16) & 127u) == 63);
    assert(((midi_out_q[1] >> 8) & 255u) == 0x80u);
    song.sel = 0;
    t->p[P_QUANT] = 0;
    fm1_in.notes = 1u << 8;                /* held black key must release after enabling mode */
    keyboard_block();
    before = mo_w;
    assert(t->arp_phys == 1);
    t->p[P_QUANT] = 2;
    fm1_in.notes = 0;
    keyboard_block();
    assert(t->arp_phys == 0 && t->nheld == 0 && mo_w == before + 1);
    puts("scales: silent keys, arp, live recording, MIDI out and held-note changes ok");
}

static void chord_shapes_test(void)
{
    track_t *t = &trk[0];
    static const uint8_t major[4][4] = {{60, 62, 67, 0}, {60, 64, 67, 74},
                                        {60, 64, 67, 69}, {60, 64, 71, 0}};
    static const uint8_t minor[4][4] = {{60, 62, 67, 0}, {60, 63, 67, 74},
                                        {60, 63, 67, 68}, {60, 63, 70, 0}};
    static const uint8_t counts[4] = {3, 4, 4, 3};
    uint8_t c[4];
    uint32_t shape, scale, root_note, n, j;
    host_tracks_init();
    t->p[P_ROOT] = 0;
    t->p[P_VLEAD] = 0;
    assert(CH_POWER == 5 && CH_SUS2 == 6 && TP[P_CHORD].max == CH_COUNT - 1);
    assert(NELEM(N_CHORD) == CH_COUNT);
    for (scale = 1; scale <= 2; scale++) {
        t->p[P_SCALE] = (int16_t)scale;
        for (shape = CH_SUS2; shape <= CH_SHELL; shape++) {
            t->p[P_CHORD] = (int16_t)shape;
            n = chord_notes(t, 60, c);
            assert(n == counts[shape - CH_SUS2]);
            assert(!memcmp(c, scale == 1 ? major[shape - CH_SUS2] : minor[shape - CH_SUS2], n));
        }
    }
    t->p[P_SCALE] = 1;
    t->p[P_CHORD] = CH_SUS2;
    n = chord_play_notes(t, 60, CM_MINOR, c); /* a suspension has no third to flip */
    assert(n == 3 && c[1] == 62);
    n = chord_play_notes(t, 60, CM_SUS4 | CM_SEVEN, c);
    assert(n == 4 && c[1] == 65 && c[3] == 71);
    t->p[P_CHORD] = CH_ADD9;
    n = chord_play_notes(t, 60, CM_MINOR | CM_NINE, c);
    assert(n == 4 && c[1] == 63 && c[3] == 74); /* no duplicate ninth */
    t->p[P_CHORD] = CH_SHELL;
    n = chord_play_notes(t, 60, CM_SEVEN | CM_NINE, c);
    assert(n == 4 && c[2] == 71 && c[3] == 74); /* no duplicate seventh */
    {
        static const uint8_t tones[][4] = {
            {0,12,0,0},{0,4,7,0},{0,3,7,0},{0,4,7,10},{0,4,7,11},
            {0,3,7,10},{0,3,6,0},{0,4,8,0},{0,3,6,10},{0,3,6,9}
        };
        static const uint8_t sizes[] = {2,3,3,4,4,4,3,3,4,4};
        assert(CH_SHELL == 9 && CH_OCTAVE == 10 && CH_COUNT == 20);
        for (scale = 0; scale < NELEM(SCALE_MASK); scale++)
            for (shape = CH_OCTAVE; shape < CH_COUNT; shape++) {
                t->p[P_SCALE] = (int16_t)scale;
                t->p[P_CHORD] = (int16_t)shape;
                n = chord_notes(t, 60, c);
                assert(n == sizes[shape - CH_OCTAVE]);
                for (j = 0; j < n; j++) assert(c[j] == 60 + tones[shape - CH_OCTAVE][j]);
            }
        t->p[P_SCALE] = 2;
        t->p[P_CHORD] = CH_MAJOR;
        n = chord_play_notes(t, 60, CM_MINOR | CM_SEVEN, c);
        assert(n == 4 && c[1] == 63 && c[3] == 71);
        t->p[P_CHORD] = CH_MINOR;
        n = chord_play_notes(t, 60, CM_MINOR | CM_SUS4 | CM_SEVEN, c);
        assert(n == 4 && c[1] == 65 && c[3] == 70);
        t->p[P_CHORD] = CH_DIM7;
        n = chord_play_notes(t, 60, CM_SEVEN | CM_NINE, c);
        assert(n == 4 && c[1] == 63 && c[2] == 69 && c[3] == 74);
        t->p[P_CHORD] = CH_HALFDIM;
        n = chord_play_notes(t, 60, CM_MINOR | CM_SEVEN | CM_NINE, c);
        assert(n == 4 && c[1] == 64 && c[2] == 70 && c[3] == 74);
        t->p[P_CHORD] = CH_AUG;
        n = chord_play_notes(t, 60, CM_SEVEN | CM_NINE, c);
        assert(n == 4 && c[1] == 64 && c[2] == 71 && c[3] == 74);
        t->p[P_CHORD] = CH_OCTAVE;
        n = chord_play_notes(t, 60, CM_MINOR | CM_SEVEN | CM_SUS4 | CM_NINE, c);
        assert(n == 2 && c[0] == 60 && c[1] == 72);
        n = chord_play_notes(t, 60, CM_INV, c);
        assert(n == 2 && c[0] == 72 && c[1] == 84);
        n = chord_play_notes(t, 104, CM_INV, c);
        assert(n == 2 && c[0] == 104 && c[1] == 116); /* preserve pair at the MIDI ceiling */
        t->p[P_VLEAD] = 1;
        for (root_note = 48; root_note <= 84; root_note++) {
            n = chord_play_notes(t, root_note, 0, c);
            assert(n == 2 && c[1] - c[0] == 12);
            n = chord_play_notes(t, root_note, CM_INV, c);
            assert(n == 2 && c[1] - c[0] == 12);
        }
        t->p[P_VLEAD] = 0;
    }
    for (shape = CH_OFF; shape < CH_COUNT; shape++)
        for (scale = 0; scale < NELEM(SCALE_MASK); scale++) {
            t->p[P_CHORD] = (int16_t)shape;
            t->p[P_SCALE] = (int16_t)scale;
            for (root_note = 100; root_note <= 127; root_note++) {
                n = chord_play_notes(t, root_note, CM_SUS4 | CM_SEVEN | CM_NINE, c);
                assert(n >= 1 && n <= 4);
                for (j = 0; j < n; j++)
                    assert(c[j] >= root_note && c[j] <= 127);
            }
        }
    t->p[P_CHORD] = CH_OFF;
    puts("chords: appended shapes, major/minor tones, modifiers and MIDI ceiling ok");
}

static uint32_t scale_voice_gates(const track_t *t) { uint32_t i, n = 0; for (i = 0; i < NVOICE; i++) n += t->v[i].gate != 0; return n; }

static void chromatic_chords_test(void)
{
    track_t *t = &trk[0];
    uint8_t notes[4];
    uint32_t key, count;
    host_tracks_init();
    t->p[P_QUANT] = 3;
    t->p[P_CHORD] = CH_TRIAD;
    t->p[P_SCALE] = 1;
    t->p[P_ROOT] = 9;                   /* ROOT does not transpose literal keys */
    t->p[P_VLEAD] = 0;
    song.octave = 0;
    assert(TP[P_QUANT].max == 3);
    for (key = 0; key < 27; key++) {
        uint32_t root = 53 + key;
        assert(kb_map(t, key) == root);
        count = chord_play_notes(t, root, CM_NINE | CM_MINOR | CM_INV, notes);
        assert(count == 3 && notes[0] == root && notes[1] == root + 4 && notes[2] == root + 7);
    }
    /* C#4 is key 8: a true C# major chord, recorded and released as owned notes. */
    song.sel = 0;
    song.rec = song.playing = 1;
    fm1_in.notes = 1u << 8;
    keyboard_block();
    assert(kb_kind[8] == KS_NOTE && kb_n[8] == 3);
    assert(kb_nt[8][0] == 61 && kb_nt[8][1] == 65 && kb_nt[8][2] == 68);
    assert(t->step[0].n == 3 && scale_voice_gates(t) == 3);
    t->p[P_QUANT] = 0;                 /* release follows old ownership after mode change */
    fm1_in.notes = 0;
    keyboard_block();
    assert(scale_voice_gates(t) == 0 && kb_kind[8] == KS_NONE);
    host_tracks_init();
    t->p[P_CHORD] = CH_TRIAD;
    t->p[P_SCALE] = 1;
    fm1_in.notes = 1u << 8;
    keyboard_block();
    assert(kb_kind[8] == KS_MOD && scale_voice_gates(t) == 0); /* legacy C# remains a ninth modifier */
    fm1_in.notes = 0;
    keyboard_block();
    puts("chords: chromatic roots, tonic quality, recording/release and legacy modifiers ok");
}

static void chord_latch_test(void)
{
    track_t *t = &trk[0];
    host_tracks_init();
    song.sel = 0;
    t->p[P_CHORD] = CH_MAJOR;
    t->p[P_QUANT] = 3;
    t->p[P_AHOLD] = 1;
    fm1_in.notes = 1u << 7;
    keyboard_block();
    fm1_in.notes = 0;
    keyboard_block();
    assert(chord_latch_n[0] == 3 && scale_voice_gates(t) == 3);
    fm1_in.notes = 1u << 9;
    keyboard_block();
    assert(chord_latch_n[0] == 0 && scale_voice_gates(t) == 3); /* replaces C with D */
    fm1_in.notes = 0;
    keyboard_block();
    assert(chord_latch_n[0] == 3 && chord_latch_notes[0][0] == 62);
    t->p[P_AHOLD] = 0;
    keyboard_block();
    assert(chord_latch_n[0] == 0 && scale_voice_gates(t) == 0);
    t->p[P_AHOLD] = 1;
    fm1_in.notes = 1u << 7; keyboard_block();
    fm1_in.notes = 0; keyboard_block();
    seq_stop();
    assert(chord_latch_n[0] == 0 && scale_voice_gates(t) == 0);
    t->p[P_QUANT] = 0;
    t->p[P_SCALE] = 1;
    t->p[P_CHORD] = CH_TRIAD;
    fm1_in.notes = 1u << 7; keyboard_block();
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_notes[0][1] == 64);
    fm1_in.notes = 1u << 1; keyboard_block(); /* F#: minor modifier */
    assert(kb_kind[1] == KS_MOD_LATCH && chord_latch_n[0] == 3);
    assert(chord_latch_notes[0][0] == 60 && chord_latch_notes[0][1] == 63 && chord_latch_notes[0][2] == 67);
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_notes[0][1] == 63 && scale_voice_gates(t) == 3); /* release preserves minor */
    fm1_in.notes = 1u << 13; keyboard_block(); /* same modifier in the upper octave toggles off */
    assert(chord_latch_notes[0][1] == 64);
    fm1_in.notes = 0; keyboard_block();
    fm1_in.notes = (1u << 3) | (1u << 5); keyboard_block(); /* seventh + sus4 */
    assert(chord_latch_n[0] == 4 && chord_latch_notes[0][1] == 65 && chord_latch_notes[0][3] == 71);
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_n[0] == 4 && chord_latch_notes[0][1] == 65 && scale_voice_gates(t) == 4);
    fm1_in.notes = 1u << 3; keyboard_block(); /* remove seventh, retain sus4 */
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_n[0] == 3 && chord_latch_notes[0][1] == 65);
    fm1_in.notes = 1u << 7; keyboard_block(); /* next root inherits toggles */
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_notes[0][1] == 65);
    song.sel = 1;
    assert(chord_mods(1) == 0 && chord_mods(0) == CM_SUS4);
    song.sel = 0;
    t->p[P_AHOLD] = 0; keyboard_block();
    assert(chord_latch_n[0] == 0 && scale_voice_gates(t) == 0 && chord_latch_mods[0] == 0);
    t->p[P_AHOLD] = 1;
    fm1_in.notes = 1u << 1; keyboard_block();
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_mods[0] == CM_MINOR); /* can prepare a quality before the root */
    t->p[P_QUANT] = 3; keyboard_block();
    assert(chord_latch_mods[0] == 0);
    t->p[P_QUANT] = 0;
    fm1_in.notes = 1u << 1; keyboard_block();
    fm1_in.notes = 0; keyboard_block();
    seq_stop();
    assert(chord_latch_mods[0] == 0);
    fm1_in.notes = 1u << 7; keyboard_block();
    fm1_in.notes |= 1u << 1; keyboard_block(); /* modifier while root is still physically held */
    assert(kb_nt[7][1] == 63);
    fm1_in.notes = 1u << 7; keyboard_block();
    assert(kb_nt[7][1] == 63);
    fm1_in.notes = 0; keyboard_block();
    assert(chord_latch_notes[0][1] == 63);
    t->p[P_AMODE] = 1; keyboard_block();
    assert(chord_latch_mods[0] == CM_MINOR && chord_latch_n[0] == 0);
    t->p[P_AMODE] = 0;
    puts("chord latch: sustain, replacement, disable and STOP cleanup ok");
    puts("chord latch: modifier toggles, combinations, root persistence, part isolation and reset ok");
}

/* 2.7: the arp modes of Felucca 1.5.1, C E G held (MIDI notes: as they are), OCT 1; ORD as pressed */
static void arp_seq(track_t *t, int mode, uint32_t n, uint8_t *out)
{
    uint32_t i;
    t->p[P_AMODE] = (int16_t)mode;
    t->arp_idx = 0xFFFFFFFFu;
    t->arp_walk_pos = 0;
    for (i = 0; i < n; i++) {
        out[i] = (uint8_t)arp_next(t);
    }
}

static void arp_modes_test(void)
{
    static const struct { int mode; uint8_t want[8]; } M[] = {
        {ARP_UP, {60, 64, 67, 60, 64, 67, 60, 64}},
        {ARP_DOWN, {67, 64, 60, 67, 64, 60, 67, 64}},
        {ARP_UPDOWN, {60, 64, 67, 64, 60, 64, 67, 64}},
        {ARP_DOWNUP, {67, 64, 60, 64, 67, 64, 60, 64}},
        {ARP_UP8, {60, 64, 67, 72, 60, 64, 67, 72}},
        {ARP_OUTSIDE, {60, 67, 64, 60, 67, 64, 60, 67}},
        {ARP_INSIDE, {64, 67, 60, 64, 67, 60, 64, 67}},
        {ARP_PINKY, {60, 67, 64, 67, 60, 67, 64, 67}},
        {ARP_THUMB, {60, 64, 60, 67, 60, 64, 60, 67}},
        {ARP_ORDER, {67, 60, 64, 67, 60, 64, 67, 60}},
        {ARP_REPEAT, {64, 64, 64, 64, 64, 64, 64, 64}},
    };
    track_t *t = &trk[0];
    uint8_t got[16];
    uint32_t i, j;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    host_tracks_init();
    assert(ARP_CHORD == 14 && TP[P_AMODE].max == ARP_COUNT-1);
    t->p[P_AOCT] = 1, t->p[P_AORDER] = 0, t->p[P_AHOLD] = 0;
    memset(held_key, 0, sizeof held_key);          /* (MIDI notes: played as they are) */
    t->held[0] = 67, t->held[1] = 60, t->held[2] = 64, t->nheld = 3;   /* pressed G, C, E */
    for (i = 0; i < sizeof M / sizeof M[0]; i++) {
        arp_seq(t, M[i].mode, 8, got);
        for (j = 0; j < 8u; j++)
            if (got[j] != M[i].want[j]) {
                fprintf(stderr, "arp mode %s step %u: %u, want %u\n", N_AMODE[M[i].mode], j, got[j], M[i].want[j]);
                assert(0);
            }
    }
    t->p[P_AOCT] = 2;                               /* UP+8 over 2 octaves: the first note over the top one */
    arp_seq(t, ARP_UP8, 7, got);
    assert(got[5] == 79 && got[6] == 84);
    t->p[P_AOCT] = 1;
    arp_seq(t, ARP_WALK, 16, got);                   /* WALK: from the bottom, one place at a time */
    assert(got[0] == 60);
    for (j = 1; j < 16u; j++)
        assert((got[j - 1] == 60 && got[j] == 64) || (got[j - 1] == 67 && got[j] == 64) ||
               (got[j - 1] == 64 && (got[j] == 60 || got[j] == 67)));
    t->p[P_AMODE] = ARP_CHORD, t->p[P_AOCT] = 2;     /* CHORD: the held notes together, an octave up every other step */
    t->arp_idx = 0xFFFFFFFFu;
    assert(arp_step(t) && t->arp_n == 3 && t->arp_notes[0] == 60 && t->arp_notes[1] == 64 && t->arp_notes[2] == 67);
    arp_release(t);
    assert(!t->arp_n);
    assert(arp_step(t) && t->arp_n == 3 && t->arp_notes[0] == 72 && t->arp_notes[1] == 76 && t->arp_notes[2] == 79);
    arp_release(t);
    t->nheld = 0;
    assert(!arp_step(t) && !t->arp_n);
    t->p[P_AMODE] = 0, t->p[P_AOCT] = 1;
    puts("arp: the 2.7 modes (Felucca 1.5.1): DNUP UP+8 CONV DIVG PINKY THUMB WALK REPEAT CHORD, ORD as pressed ok");
}

/* 2.7 (#52): ARP SWG with the transport stopped: the odd steps late by SWG, the even ones back on the grid */
static void arp_swing_test(void)
{
    track_t *t = &trk[0];
    uint32_t div = 2, u = div_units(div), sw, pos = 0, last = 0, gaps[6], ng = 0, idx0, i, ok = 1;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    host_tracks_init();
    memset(held_key, 0, sizeof held_key);
    song.playing = 0;
    t->p[P_AMODE] = ARP_UP, t->p[P_ARATE] = (int16_t)div, t->p[P_AOCT] = 1, t->p[P_AGATE] = 64, t->p[P_APROB] = 127;
    t->held[0] = 60, t->nheld = 1;
    for (uint32_t swg = 100; swg >= 50u; swg -= 50u) {  /* (SWG 50 too: the first step's length) */
    t->p[P_ASWING] = (int16_t)swg;
    sw = swing_units(swg, u);
    t->arp_new = 1, t->arp_idx = 0xFFFFFFFFu, ng = 0, pos = last = 0, ok = 1;
    idx0 = t->arp_idx;
    for (i = 0; i < 400000u && ng < 6u; i++) {         /* 100 units a tick */
        uint32_t before = t->arp_idx;
        arp_tick(t, 100);
        pos += 100;
        if (t->arp_idx != before) {
            if (before != idx0)
                gaps[ng++] = pos - last;
            last = pos;
        }
    }
    for (i = 0; i < ng; i++)                            /* to the odd step u + sw, to the even one u - sw (100 a tick) */
        if ((i & 1u ? u - sw : u + sw) + 100u < gaps[i] || gaps[i] + 100u < (i & 1u ? u - sw : u + sw))
            ok = 0;
    printf("arp swing stopped SWG %u: steps %u %u %u %u (u %u, sw %u)\n", swg, gaps[0], gaps[1], gaps[2], gaps[3], u, sw);
    assert(ng == 6u && ok && sw > 0u);
    }
    t->p[P_ASWING] = 0, t->arp_new = 1, t->arp_idx = 0xFFFFFFFFu, ng = 0, pos = last = 0;
    idx0 = t->arp_idx;
    for (i = 0; i < 400000u && ng < 4u; i++) {
        uint32_t before = t->arp_idx;
        arp_tick(t, 100);
        pos += 100;
        if (t->arp_idx != before) {
            if (before != idx0)
                gaps[ng++] = pos - last;
            last = pos;
        }
    }
    for (i = 0; i < ng; i++)
        assert(gaps[i] + 100u >= u && gaps[i] <= u + 100u);   /* SWG 0: as before, every step u */
    t->nheld = 0;
    arp_tick(t, 100);
    t->p[P_AMODE] = 0;
    puts("arp: SWG works with the transport stopped (odd steps late), SWG 0 as before ok");
}

/* 2.7 (#106): CLK OUT: 24 pulses a beat, START with the downbeat's pulse, STOP; nothing with SYNC USB or CLK OUT OFF */
static void clkout_test(void)
{
    uint32_t n = 32, i, f8 = 0, fa = 0, fc = 0, other = 0, beats = 4, blocks;
    memset(&song, 0, sizeof song);
    host_tracks_init();
    usb.config = 1;
    song.g[G_BPM] = 120;
    song.g[G_CLKOUT] = 1, song.g[G_SYNC] = 0;
    mo_w = mo_r = 0;
    blocks = beats * BEAT_U / (n * 120u);
    for (i = 0; i < blocks; i++) {
        clkout_block(n);
        while (mo_r != mo_w) {
            uint32_t b = (midi_out_q[mo_r++ % MOUT_Q] >> 8) & 0xFFu;
            f8 += b == 0xF8u, fa += b == 0xFAu, fc += b == 0xFCu, other += b != 0xF8u && b != 0xFAu && b != 0xFCu;
        }
    }
    printf("clk out stopped, 4 beats: %u pulses\n", f8);
    assert(f8 >= 24u * beats - 1u && f8 <= 24u * beats && !fa && !fc && !other);
    song.playing = 1;
    f8 = 0;
    clkout_block(n);
    while (mo_r != mo_w) {
        uint32_t b = (midi_out_q[mo_r++ % MOUT_Q] >> 8) & 0xFFu;
        if (b == 0xFAu) fa++, assert(f8 == 0u);          /* START first */
        f8 += b == 0xF8u;
    }
    assert(fa == 1u && f8 == 1u);                       /* START, then the downbeat's pulse */
    clkout_block(n);
    song.playing = 0;
    clkout_block(n);
    while (mo_r != mo_w)
        fc += ((midi_out_q[mo_r++ % MOUT_Q] >> 8) & 0xFFu) == 0xFCu;
    assert(fc == 1u);
    song.g[G_SYNC] = 1;                                 /* following a clock: nothing out */
    for (i = 0; i < blocks; i++)
        clkout_block(n);
    assert(mo_r == mo_w);
    song.g[G_SYNC] = 0, song.g[G_CLKOUT] = 0;
    song.playing = 1;
    for (i = 0; i < blocks; i++)
        clkout_block(n);
    assert(mo_r == mo_w);                               /* CLK OUT OFF: nothing (as before) */
    song.playing = 0;
    song.g[G_CLKOUT] = 1, song.playing = 1;
    clkout_block(n);
    song.g[G_CLKOUT] = 0;                               /* turned off while playing: STOP */
    clkout_block(n);
    fc = 0;
    while (mo_r != mo_w)
        fc += ((midi_out_q[mo_r++ % MOUT_Q] >> 8) & 0xFFu) == 0xFCu;
    assert(fc == 1u);
    song.playing = 0;
    clkout_block(n);
    puts("clk out: 24 pulses a beat (stopped too), START + downbeat pulse, STOP; none with SYNC USB or OFF ok");
}

/* 2.7 (#103): KBD HOLD (ARP 2 HOLD ON, MODE OFF): the notes keep sounding; a new chord replaces them; keys join while
 * down; the MIDI note-off waits; HOLD OFF ends them. MODE OFF, HOLD OFF: as before */
static int sounding(const track_t *t, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].gate && t->v[i].note == note)
            return 1;
    return 0;
}
static uint32_t midi_offs(uint32_t note)
{
    uint32_t c = 0;
    while (mo_r != mo_w) {
        uint32_t pk = midi_out_q[mo_r++ % MOUT_Q];
        c += ((pk >> 8) & 0xF0u) == 0x80u && ((pk >> 16) & 127u) == note;
    }
    return c;
}
static void kbd_hold_test(void)
{
    track_t *t = &trk[0];
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    host_tracks_init();
    memset(held_key, 0, sizeof held_key);
    kb_prev = 0;
    usb.config = 1;
    song.g[G_CH1] = 1;
    mo_w = mo_r = 0;
    t->p[P_AMODE] = 0, t->p[P_AHOLD] = 1, t->p[P_QUANT] = 0, t->p[P_CHORD] = 0, t->p[P_VOICE] = V_POLY;
    fm1_in.notes = 1u << 7 | 1u << 11;                  /* C4 + E4 down */
    keyboard_block();
    assert(sounding(t, 60) && sounding(t, 64));
    fm1_in.notes = 0;                                   /* let go: they go on, no MIDI note-off yet */
    keyboard_block();
    assert(sounding(t, 60) && sounding(t, 64) && kh_has(t, 60) && midi_offs(60) == 0u);
    fm1_in.notes = 1u << 14;                            /* G4 alone: the new chord replaces C E */
    keyboard_block();
    assert(!sounding(t, 60) && !sounding(t, 64) && sounding(t, 67) && midi_offs(60) == 1u);
    fm1_in.notes = 1u << 14 | 1u << 18;                 /* B4 while G4 is down: joins */
    keyboard_block();
    fm1_in.notes = 0;
    keyboard_block();
    assert(sounding(t, 67) && sounding(t, 71));
    t->p[P_AHOLD] = 0;                                  /* HOLD OFF: they end (the block's check) */
    if (kh_any(t) && !kh_on(t))
        kh_release(t);
    assert(!sounding(t, 67) && !sounding(t, 71) && !kh_any(t));
    fm1_in.notes = 1u << 7;                             /* HOLD OFF: as before */
    keyboard_block();
    fm1_in.notes = 0;
    keyboard_block();
    assert(!sounding(t, 60));
    t->p[P_AMODE] = 1, t->p[P_AHOLD] = 1;               /* ARP on + HOLD: the arp's latch, not KBD HOLD */
    fm1_in.notes = 1u << 7;
    keyboard_block();
    fm1_in.notes = 0;
    keyboard_block();
    assert(!kh_any(t) && t->nheld == 1u);
    t->p[P_AMODE] = 0, t->p[P_AHOLD] = 0, t->nheld = 0;
    puts("kbd hold: notes held when let go, a new chord replaces them, keys join while down, MIDI off waits, HOLD OFF ends ok");
}

/* 2.7 (#126): EDIT's MUTE mode (drum track) stays when EDIT is let go */
static void mute_mode_test(void)
{
    er_mute = 1;
    fm1_in.buttons = 0, fm1_in.notes = 0;
    keyboard_block();
    assert(er_mute == 1u);
    er_mute = 0;
    puts("edit mute mode: kept when EDIT is let go ok");
}

int main(void)
{
    host_tracks_init();
    mapping_test();
    chord_shapes_test();
    key_events_test();
    chromatic_chords_test();
    chord_latch_test();
    arp_modes_test();
    arp_swing_test();
    clkout_test();
    kbd_hold_test();
    mute_mode_test();
    return 0;
}
