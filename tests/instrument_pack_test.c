/* SPDX-License-Identifier: GPL-3.0-only */
/* Render real new instruments at different pitches/velocities and after release.
 * Optional argument writes labelled listening reels, not a second synth model. */
#undef NDEBUG
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

static const struct { uint8_t engine, preset; const char *name; } pack[] = {
    {3,8,"NES PLS12"}, {3,9,"NES PLS50"}, {3,10,"NES PLUCK"}, {3,11,"NES TRI"},
    {3,12,"GB WAVE BS"}, {3,13,"GB HOLLOW"}, {3,14,"GB ORGAN"}, {3,15,"GB ARP"},
    {2,8,"VIOLIN"}, {1,22,"CLARINET"}, {2,9,"OBOE"}, {1,23,"BASSOON"},
    {10,26,"HARPSICHORD"}, {10,27,"OUD"},
    {3,16,"DUST KEYS"}, {3,17,"WORN PAD"}, {3,18,"TOY BELL"}, {3,19,"DUST BASS"}
};
static void load(unsigned i)
{
    host_tracks_init(); song.playing = 0;
    rng_state = 0x1234567;
    assert(pack[i].preset < ENGINES[pack[i].engine]->npresets);
    assert(!strcmp(ENGINES[pack[i].engine]->presets[pack[i].preset].name, pack[i].name));
    host_preset(&trk[0], pack[i].engine, pack[i].preset);
}
static uint64_t dry_render(unsigned i, unsigned note, unsigned velocity)
{
    load(i); track_t *t = &trk[0];
    t->p[P_DIST] = t->p[P_CHOR] = t->p[P_DLY] = t->p[P_REV] = 0;
    trk_note_on(t, note, velocity);
    uint64_t energy = 0, hash = 1469598103934665603ull;
    for (unsigned b = 0; b < 2u * FS / CTL; b++) {
        int32_t out[CTL];
        if (b == FS / (2u * CTL)) trk_note_off(t, note);
        track_render(t, out, CTL);
        for (unsigned j = 0; j < CTL; j++) {
            assert(out[j] > -1000000 && out[j] < 1000000);
            if (b < FS / (2u * CTL)) energy += (uint32_t)abs(out[j]);
            hash = (hash ^ (uint32_t)out[j]) * 1099511628211ull;
        }
    }
    assert(energy > 10000);
    for (unsigned v = 0; v < NVOICE; v++) assert(!t->v[v].active && !t->v[v].gate);
    return hash;
}
static void reels(const char *directory)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/instrument-pack.txt", directory);
    FILE *list = fopen(path, "w"); assert(list);
    const char *reel[]={"nes-gameboy","instruments","dusty-presets"};
    for (unsigned group = 0; group < 3; group++) {
        snprintf(path, sizeof path, "%s/%s.wav", directory, reel[group]);
        FILE *f = fopen(path, "wb"); assert(f); wav_hdr(f, 0);
        unsigned frames = 0, start = group==0?0:group==1?8:14, end = group==0?8:group==1?14:NELEM(pack);
        for (unsigned i = start; i < end; i++) {
            fprintf(list, "%s %.3f %s\n", reel[group], (double)frames / FS, pack[i].name);
            load(i);
            static const uint8_t notes[] = {60, 64, 67, 72};
            for (unsigned b = 0; b < 3u * FS / CTL; b++) {
                unsigned position = b * CTL;
                for (unsigned n = 0; n < NELEM(notes); n++) {
                    unsigned at = n * FS / 3;
                    if (at >= position && at < position + CTL) trk_note_on(&trk[0], notes[n], 100);
                    at += FS / 4;
                    if (at >= position && at < position + CTL) trk_note_off(&trk[0], notes[n]);
                }
                int32_t out[2 * CTL]; mix_block(out, CTL);
                for (unsigned j = 0; j < CTL; j++) wav_put(f, out[2*j], out[2*j+1]);
                frames += CTL;
            }
        }
        fseek(f, 0, SEEK_SET); wav_hdr(f, frames); assert(!fclose(f));
    }
    assert(!fclose(list));
}
int main(int argc, char **argv)
{
    uint64_t signatures[NELEM(pack)];
    for (unsigned i = 0; i < NELEM(pack); i++) {
        signatures[i] = dry_render(i, 60, 100);
        for (unsigned j = 0; j < i; j++) assert(signatures[i] != signatures[j]);
        for (unsigned note = 48; note <= 84; note += 12)
            for (unsigned vel = 40; vel <= 127; vel += 87) dry_render(i, note, vel);
        printf("instrument: %s range/velocity/release PASS\n", pack[i].name);
    }
    /* Exercise the shared voice budget with new synths plus the drum track. */
    host_tracks_init();
    host_preset(&trk[0], 3, 15);
    host_preset(&trk[1], 2, 8);
    host_preset(&trk[2], 10, 27);
    for (unsigned k = 0; k < 3; k++) {
        trk_note_on(&trk[k], 60 + k * 4, 100);
        trk_note_on(&trk[k], 67 + k * 4, 100);
    }
    uint64_t mixed_energy = 0;
    for (unsigned b = 0; b < 3u * FS / CTL; b++) {
        int32_t out[2 * CTL];
        if (b == FS / CTL) for (unsigned k = 0; k < 3; k++) {
            trk_note_off(&trk[k], 60 + k * 4);
            trk_note_off(&trk[k], 67 + k * 4);
        }
        if (b < FS / CTL && b % 256 == 0) trk_note_on(&trk[3], 36, 100);
        mix_block(out, CTL);
        for (unsigned j = 0; j < 2 * CTL; j++) {
            assert(out[j] >= -32768 && out[j] <= 32767);
            mixed_energy += abs(out[j]);
        }
    }
    assert(mixed_energy > 10000);
    for (unsigned k = 0; k < 3; k++) for (unsigned v = 0; v < NVOICE; v++)
        assert(!trk[k].v[v].active && !trk[k].v[v].gate);
    if (argc > 1) reels(argv[1]);
    puts("instrument pack: eighteen distinct real-engine voices with bounded output and clean releases PASS");
    return 0;
}
