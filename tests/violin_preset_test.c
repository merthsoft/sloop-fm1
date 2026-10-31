/* SPDX-License-Identifier: GPL-3.0-only */
/* The factory violin should retain a useful sustained fundamental while adding
 * bow-like harmonic edge. Compare the former recipe at matched gain, then check
 * soft/hard notes across G3-C6 for clipping, stuck voices and release silence. */
#define main hostsim_main
#include "hostsim.c"
#undef main
#include <assert.h>

typedef struct { double energy, difference; int32_t peak, tail; } violin_measure;

static violin_measure render_violin(int previous, uint8_t note, uint8_t velocity)
{
    host_tracks_init();
    host_preset(&trk[0], 2, 8);
    assert(!strcmp(ENGINES[2]->presets[8].name, "VIOLIN"));
    if (previous) {
        trk[0].p[P_E2] = 38;
        trk[0].p[P_E3] = 12;
    }
    violin_measure m = {0};
    int32_t last = 0;
    trk_note_on(&trk[0], note, velocity);
    for (uint32_t b = 0; b < 5 * FS / CTL; ++b) {
        if (b == 3 * FS / CTL) trk_note_off(&trk[0], note);
        int32_t out[2 * CTL];
        mix_block(out, CTL);
        for (uint32_t i = 0; i < CTL; ++i) {
            uint32_t frame = b * CTL + i;
            int32_t x = (out[2*i] + out[2*i+1]) / 2;
            for (uint32_t c = 0; c < 2; ++c) {
                int32_t a = abs(out[2*i+c]);
                if (a > m.peak) m.peak = a;
                if (frame > 19 * FS / 4 && a > m.tail) m.tail = a;
            }
            if (frame >= FS && frame < 2 * FS) {
                m.energy += (double)x * x;
                m.difference += (double)(x-last) * (x-last);
            }
            last = x;
        }
    }
    for (uint32_t i = 0; i < NVOICE; ++i) assert(!trk[0].v[i].active);
    assert(m.peak > 100 && m.peak < 30000);
    assert(m.tail <= 6);
    return m;
}

static violin_measure isolated(int previous, uint8_t note, uint8_t velocity)
{
    int fd[2]; assert(!pipe(fd));
    pid_t child = fork(); assert(child >= 0);
    if (!child) {
        close(fd[0]);
        violin_measure m = render_violin(previous, note, velocity);
        assert(write(fd[1], &m, sizeof m) == sizeof m);
        close(fd[1]); _exit(0);
    }
    close(fd[1]); violin_measure m;
    assert(read(fd[0], &m, sizeof m) == sizeof m); close(fd[0]);
    int status; assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status) && !WEXITSTATUS(status));
    return m;
}

int main(void)
{
    violin_measure before = isolated(1, 69, 100), after = isolated(0, 69, 100);
    double edge = (after.difference / after.energy) / (before.difference / before.energy);
    assert(edge > 1.5);                     /* More edge, not just more gain. */
    assert(after.energy > before.energy * .8 && after.energy < before.energy * 1.25);
    const uint8_t notes[] = {55, 60, 69, 76, 84}, velocities[] = {32, 80, 127};
    for (uint32_t n = 0; n < sizeof notes; ++n)
        for (uint32_t v = 0; v < sizeof velocities; ++v)
            isolated(0, notes[n], velocities[v]);
    printf("VIOLIN: normalized harmonic-edge ratio %.3f; gain, G3-C6 velocities, release and clipping PASS\n", edge);
    return 0;
}
