/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static void pump(uint32_t blocks)
{
    int32_t out[CTL * 2];
    while (blocks--) mix_block(out, CTL);
}
static void setup(uint32_t engine)
{
    static int initialized;
    if (initialized) {
        for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
        pump(FS / CTL * 20);
    }
    host_tracks_init();
    initialized = 1;
    for (uint32_t p = 0; p < NPART; p++) {
        host_preset(&trk[p], engine, 0);
        trk[p].p[P_VOICE] = V_POLY;
        trk[p].p[P_AMODE] = 0;
        trk[p].p[P_SUS] = 127;
        sustain_p[p][0] = 0;
    }
}
static void chord(uint32_t part, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) trk_note_on(&trk[part], 48 + i, 100);
}
int main(void)
{
    /* Assert audible capacity, not just the allocator's own accounting. */
    for (uint32_t engine = 3; engine <= 7; engine += 4) {
        setup(engine);
        chord(0, 6); chord(1, 6); pump(4);
        assert(voices_busy() == 12);
        trk_note_on(&trk[2], 72, 100); pump(4);
        assert(voices_busy() == 12 && trk[0].v[0].gate && trk[1].v[0].gate);
        for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
    }
    /* A fading voice is outside admission; retriggering it must buy room again. */
    setup(3); chord(0, 8); chord(1, 4); shed_voice();
    uint32_t fading = NVOICE;
    for (uint32_t i = 0; i < NVOICE; i++) if (trk[0].v[i].stage == 4) fading = i;
    assert(fading < NVOICE);
    trk_note_on(&trk[2], 72, 100);
    trk_note_on(&trk[0], trk[0].v[fading].note, 100);
    assert(voice_load() <= 24);
    setup(ENGI_FM6); chord(0, 8); pump(4);
    assert(voices_busy() == 8 && trk_nvoice(&trk[0]) == 8);
    for (uint32_t i = 0; i < 8; i++) assert(trk[0].v[i].active && fm6_note_of(&trk[0], &trk[0].v[i])->down);
    /* Released pedal tails yield before held bass/lead, with mixed engine costs. */
    setup(3); sustain_p[0][0] = 1; sustain_p[0][1] = 7;
    chord(0, 8); pump(4);
    for (uint32_t i = 0; i < 8; i++) trk_note_off(&trk[0], 48 + i);
    host_preset(&trk[1], ENGI_FM6, 0); trk[1].p[P_VOICE] = V_POLY; trk[1].p[P_AMODE] = 0;
    chord(1, 8); pump(4);
    assert(voices_busy() == 8);
    for (uint32_t i = 0; i < 8; i++) assert(!trk[0].v[i].active && trk[1].v[i].gate);
    /* Reattacks, engine changes and overload fades must never bypass admission. */
    static const uint32_t engines[] = {3, 7, ENGI_FM6, 0, 9, 10};
    for (uint32_t scenario = 0; scenario < 6; scenario++) {
        setup(engines[scenario]);
        for (uint32_t p = 0; p < NPART; p++) {
            sustain_p[p][0] = 1; sustain_p[p][1] = 7;
        }
        for (uint32_t k = 0; k < 1600; k++) {
            uint32_t p = k % NPART, n = 48 + (k * 17) % 24;
            trk_note_on(&trk[p], n, 100);
            assert(voice_load() <= 24);
            if (k % 3 == 0) trk_note_off(&trk[p], n);
            if (k % 17 == 0) shed_voice();
            pump(1);
        }
        for (uint32_t p = 0; p < NPART; p++) trk_all_off(&trk[p]);
        pump(FS / CTL * 20);
        assert(!voices_busy());
    }
    if (getenv("CORES") && atoi(getenv("CORES")) == 2) assert(cpu1_host_jobs() > 0);
    puts("voice budget: twelve cheap, eight FM6, mixed sustain stealing, reattacks and cleanup PASS");
}
