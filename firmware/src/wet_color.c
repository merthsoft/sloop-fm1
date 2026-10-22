/* SPDX-License-Identifier: GPL-3.0-only */
/* Include-style DSP module. int32 wet buses, nominal full scale +/-32768.
 * No globals, allocation, RNG or delay line. Zero initialization is OFF.
 * Call once per frame (mono: channels=1); do not share state between buses.
 * Settings may change per frame. Audio-thread ownership is required. */
#include <stdint.h>

enum { WET_COLOR_OFF, WET_COLOR_TAPE, WET_COLOR_CRUSH };
typedef struct {
    int32_t low[2], held[2];
    int32_t tape, crush;                 /* Q15 crossfade weights */
    uint32_t phase;
} wet_color_t;

static int32_t wet_color_slew(int32_t x, int32_t target)
{
    int32_t d = target - x;
    return x + (d > 64 ? 64 : d < -64 ? -64 : d);
}

/* amount: 0..127; invalid modes safely request OFF. channels: 1 or 2.
 * frame must have channels entries. n/a channels is a no-op. */
static void wet_color_frame(wet_color_t *s, int32_t *frame,
                            uint32_t channels, uint32_t mode, uint32_t amount)
{
    uint32_t c;
    int32_t target;
    if (channels < 1 || channels > 2) return;
    if (amount > 127) amount = 127;
    target = (int32_t)(amount * 258u);   /* endpoint corrected below */
    if (amount == 127) target = 32768;
    s->tape = wet_color_slew(s->tape, mode == WET_COLOR_TAPE ? target : 0);
    s->crush = wet_color_slew(s->crush, mode == WET_COLOR_CRUSH ? target : 0);
    if (!(s->tape | s->crush)) {
        s->low[0] = s->low[1] = s->held[0] = s->held[1] = 0;
        s->phase = 0;
        return;                        /* exact OFF, even outside nominal range */
    }
    for (c = 0; c < channels; ++c) {
        int32_t dry = frame[c];
        int32_t x = dry > 32768 ? 32768 : dry < -32768 ? -32768 : dry;
        int32_t a = x < 0 ? -x : x;
        /* Odd, non-expansive quadratic knee; bounded products fit int32. */
        int32_t tape = x - x * a / 65536;
        int32_t delta = tape - s->low[c];
        s->low[c] += (delta + (delta > 0 ? 7 : delta < 0 ? -7 : 0)) / 8;
        if (!s->phase) {
            /* Symmetric rounding, 8-bit signed grid, no DC bias at zero. */
            int32_t q = (a + 128) / 256;
            s->held[c] = x < 0 ? -q * 256 : q * 256;
        }
        /* 64-bit difference/mix protects INT32_MIN/MAX input and transitions.
         * Both weights slew equally, so their sum never exceeds unity. */
        frame[c] = (int32_t)((int64_t)dry +
            (((int64_t)s->low[c] - dry) * s->tape +
             ((int64_t)s->held[c] - dry) * s->crush) / 32768);
    }
    s->phase = (s->phase + 1u) & 3u;
}
