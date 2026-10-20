/* SPDX-License-Identifier: GPL-3.0-only */
/* Controlled factory-sound audit. Each render starts in a fresh child process,
 * so voices, FX, RNG and engine arenas cannot leak between presets.
 * Build as preset_level.c; argv[1] is an output directory. Raw files are
 * interleaved stereo int32 at FS, 3 seconds held + 1 second release, velocity 100.
 * No sequencer/arp is run: these are direct key presses with factory sound/FX. */
#define main hostsim_main
#include "hostsim.c"
#undef main

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    for (uint32_t e = 0; e < NENGINES; ++e)
        for (uint32_t p = 0; p < ENGINES[e]->npresets; ++p)
            for (uint32_t mode = 0; mode < 4; ++mode) {
                pid_t child = fork();
                if (child < 0) return 3;
                if (!child) {
                    char path[1024];
                    snprintf(path, sizeof path, "%s/%u-%u-%u.raw", argv[1], e, p, mode);
                    FILE *f = fopen(path, "wb");
                    if (!f) _exit(4);
                    host_tracks_init();
                    host_preset(&trk[0], e, p);
                    const uint8_t chord[] = {60, 64, 67, 71};
                    uint32_t count = mode == 3 ? 4 : 1;
                    for (uint32_t i = 0; i < count; ++i)
                        trk_note_on(&trk[0], mode == 3 ? chord[i] : 48 + 12 * mode, 100);
                    for (uint32_t b = 0; b < 4 * FS / CTL; ++b) {
                        int32_t out[2 * CTL];
                        if (b == 3 * FS / CTL)
                            for (uint32_t i = 0; i < count; ++i)
                                trk_note_off(&trk[0], mode == 3 ? chord[i] : 48 + 12 * mode);
                        mix_block(out, CTL);
                        if (fwrite(out, sizeof out, 1, f) != 1) _exit(5);
                    }
                    if (fclose(f)) _exit(6);
                    _exit(0);
                }
                int status;
                if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status)) return 7;
            }
    return 0;
}
