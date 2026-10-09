/* SPDX-License-Identifier: GPL-3.0-only */
/* PUNCH-IN FX, pocket-operator style: hold FX, press a white key (16 of them, F3..G5) and the
 * whole mix goes through that effect while the key is held; release it and the mix comes
 * back. Beat-synced to the tempo; loops start on the grid of the running transport.
 * A mono ring of the mix (PUNCH_N samples, 0.74 s) feeds the loops, reverse, tape stop,
 * half speed, wobble and echo; the filters, crush and gate run in stereo. Every change
 * crossfades over 64 samples. Runs in the audio ISR (mix_block, fx.c). */
#include "performance_remote.h"
#define PUNCH_N 32768u                    /* power of two */
/* the transport clock (core.h clk_pos) places the loops and the gate on the grid */
static uint32_t clk_samples(void) { return clk_pos / (uint32_t)song.g[G_BPM]; }   /* samples into the beat */
#define PUNCH_NFX 16u
enum { PX_LOOP4, PX_LOOP8, PX_LOOP16, PX_LOOP32, PX_STUT, PX_REV, PX_STOP, PX_HALF,
       PX_LPF, PX_HPF, PX_TEL, PX_CRUSH, PX_DOWN, PX_GATE, PX_ECHO, PX_WOBBLE };
static const char *const PUNCH_NAME[PUNCH_NFX] = {
    "LOOP 4", "LOOP 8", "LOOP 16", "LOOP 32", "STUTTER", "REVERSE", "STOP", "HALF",
    "LOW", "HIGH", "PHONE", "CRUSH", "ALIAS", "GATE", "ECHO", "WOBBLE"};
static int16_t punch_ring[PUNCH_N] __attribute__((section(".pool")));
static struct {
    volatile int8_t req;          /* effect asked for by the keys (-1 none), ISR keyboard_block */
    volatile uint8_t hold;        /* FX button held (UI main loop) */
    volatile uint32_t black_keys;         /* momentary black-key modifiers */
    uint8_t mods, retrigger, layer_seen;
    volatile uint8_t latch;      /* UI reads state written by the audio ISR */
    uint32_t base_len;
    uint32_t keybit;              /* the key that started it */
    int8_t cur;                   /* effect playing (fading out when req differs) */
    int32_t g;                    /* wet gain Q15 */
    uint32_t w;                   /* ring write index */
    uint32_t start, t, len;       /* loop / reverse: start index, samples since, length */
    uint32_t pre;                 /* loops: samples of dry still to go to the next division */
    uint32_t k, sub;              /* position in the loop; HALF: odd / even sample */
    uint32_t dspd;                /* tape stop: speed step per sample */
    uint32_t gp;                  /* gate: samples into the 1/16 step */
    uint32_t rp;                  /* tape stop / wobble: read position Q16 */
    uint32_t spd;                 /* tape stop speed Q16 */
    uint32_t lfo;                 /* wobble phase */
    int32_t held_l, held_r, hn;   /* downsample */
    int32_t f1l, f2l, f1r, f2r, f3l, f4l, f3r, f4r;   /* filter states */
    int32_t cut;                  /* sweep, 0..127 << 8 */
} punch = {.req = -1, .cur = -1};

enum { PM_SLOW = 1, PM_FAST = 2, PM_TRIP = 4, PM_SOFT = 8, PM_HARD = 16, PM_BLEND = 32,
       PM_LATCH = 64, PM_RETRIGGER = 128 };
static uint32_t punch_modifier(uint32_t k)
{
    switch (k) {
    case 1: return PM_SLOW; case 3: return PM_FAST; case 5: return PM_TRIP;
    case 8: case 20: return PM_SOFT; case 10: case 22: return PM_HARD;
    case 13: case 25: return PM_BLEND; case 15: return PM_LATCH; case 17: return PM_RETRIGGER;
    default: return 0;
    }
}
static uint32_t punch_mods(void)
{
    uint32_t b = punch.black_keys;
    return (b & (1u << 1) ? PM_SLOW : 0) | (b & (1u << 3) ? PM_FAST : 0) |
        (b & (1u << 5) ? PM_TRIP : 0) | (b & ((1u << 8) | (1u << 20)) ? PM_SOFT : 0) |
        (b & ((1u << 10) | (1u << 22)) ? PM_HARD : 0) |
        (b & ((1u << 13) | (1u << 25)) ? PM_BLEND : 0);
}
static void punch_clear(void)
{
    punch.req = -1; punch.keybit = punch.black_keys = 0;
    punch.latch = punch.retrigger = punch.mods = 0;
}
static void punch_modifier_event(uint32_t k, int on)
{
    uint32_t m = punch_modifier(k);
    if (m == PM_LATCH) {
        if (on && punch.req >= 0) {
            punch.latch ^= 1;
            if (!punch.latch && !punch.keybit) punch.req = -1;
        }
    } else if (m == PM_RETRIGGER) {
        if (on && punch.req >= 0) punch.retrigger = 1;
    } else if (on) punch.black_keys |= 1u << k;
    else punch.black_keys &= ~(1u << k);
}
static uint32_t punch_division(uint32_t n, uint32_t mods)
{
    if (mods & PM_TRIP) n = n * 2u / 3u;
    if ((mods & (PM_SLOW | PM_FAST)) == PM_SLOW) n *= 2u;
    if ((mods & (PM_SLOW | PM_FAST)) == PM_FAST) n /= 2u;
    return n < 64u ? 64u : n;
}

static uint32_t beat_samples(void) { return (uint32_t)FS * 60u / (uint32_t)song.g[G_BPM]; }

static int32_t ring_raw(uint32_t i) { return punch_ring[i & (PUNCH_N - 1u)]; }   /* the stored 16 bits (mix / 8) */
static int32_t ring_at(uint32_t i) { return ring_raw(i) << 3; }
static int32_t ring_q16(uint32_t p)                 /* read at Q16 position, linear (on the stored values: no overflow) */
{
    int32_t a = ring_raw(p >> 16), b = ring_raw((p >> 16) + 1u);
    return (a + (((b - a) * (int32_t)((p >> 1) & 0x7FFFu)) >> 15)) << 3;
}

static void punch_start(int32_t fx)
{
    uint32_t beat = beat_samples(), len, ph = clk_samples();
    static const uint8_t DIV[8] = {1, 2, 4, 8, 6, 1, 1, 2};   /* loops: beat / DIV */
    punch.cur = (int8_t)fx;
    punch.t = 0;
    len = fx <= PX_HALF ? beat / DIV[fx] : beat;
    if (len > PUNCH_N - 1024u)
        len = PUNCH_N - 1024u;
    if (len < 64u)
        len = 64u;
    punch.base_len = len;
    punch.len = len = punch.mods & PM_TRIP ? len * 2u / 3u : len;
    punch.pre = 0;
    punch.start = punch.w;
    if (fx <= PX_STUT && song.playing) {                /* on the grid: */
        uint32_t back = ph % len;
        if (len - back <= 1536u) {                      /* just before a division: from that one */
            punch.pre = len - back;
        } else {                                        /* else from the last division start */
            punch.start = punch.w - back;
            punch.t = back;
        }
    }
    punch.rp = punch.w << 16;
    punch.spd = 65536u;
    punch.dspd = 65536u / len + 1u;
    punch.sub = 0;
    punch.k = punch.t % len;
    punch.lfo = 0;
    punch.hn = 0;
    punch.cut = 127 << 8;
    punch.f1l = punch.f2l = punch.f1r = punch.f2r = punch.f3l = punch.f4l = punch.f3r = punch.f4r = 0;
}

/* the wet sample for the mono ring effects (after the ring got this sample); no divide per
 * sample: k is the position in the loop, counted up and wrapped */
static int32_t punch_ring_fx(int32_t fx, uint32_t rate, uint32_t wobble_inc, int32_t depth)
{
    uint32_t len = punch.len, k = punch.k;
    int32_t y;
    switch (fx) {
    case PX_REV:
        y = ring_at(punch.start - k);
        break;
    case PX_STOP:
        y = ring_q16(punch.rp);
        punch.rp += (punch.spd >> 2) * (rate >> 14);
        punch.spd = punch.spd > punch.dspd ? punch.spd - punch.dspd : 0u;   /* to a stop in a beat */
        return ((y >> 3) * (int32_t)(punch.spd >> 4)) >> 9;
    case PX_WOBBLE: {
        int32_t d = 700 + (sine_i(punch.lfo) * depth >> 15);
        punch.lfo += wobble_inc;           /* 4 Hz */
        return ring_q16((punch.w << 16) - ((uint32_t)d << 16));
    }
    default:                                        /* loops, HALF */
        y = ring_at(punch.start + k);
        break;
    }
    punch.sub += fx == PX_HALF ? rate >> 1 : rate;
    punch.k = k + (punch.sub >> 16);
    punch.sub &= 65535u;
    if (punch.k >= len) punch.k -= len;
    /* declick at the seams */
    return k < 64u ? (y * (int32_t)k) >> 6 : len - k < 64u ? (y * (int32_t)(len - k)) >> 6 : y;
}

/* the loops, STUTTER, HALF and REVERSE replay ring samples [start, start + len) (REVERSE: up to start,
 * backwards). The ring keeps recording the mix, and after PUNCH_N samples (0.74 s) the writer comes
 * round into that span: it skips those slots once the loop is captured, so a held loop plays for as
 * long as the key is held. The rest of the ring keeps recording (the next effect has its past). */
static int punch_owns(int32_t fx, uint32_t w)
{
    uint32_t d = w - punch.start, len = punch.len;
    if (fx < 0 || fx > PX_HALF || fx == PX_STOP || punch.pre)
        return 0;
    if (fx == PX_REV)
        return d >= 1u && ((d + len - 1u) & (PUNCH_N - 1u)) < len;
    return d >= len && (d & (PUNCH_N - 1u)) < len;
}

/* l, r: the mix before the master (fx.c mix_block), n samples, in place */
static void punch_process(int32_t *l, int32_t *r, uint32_t n)
{
    uint32_t i;
    int32_t want = punch.req >= 0 ? punch.req : perf_punch(fm1_ms);
    uint32_t beat, step, echo_d, mods, rate, wet, wobble_inc;
    int32_t strength, depth, crush, down, feedback;
    tsvf_t c1, c2;
    if (punch.cur < 0 && want < 0) {                   /* idle: only the ring */
        for (i = 0; i < n; i++)
            punch_ring[punch.w++ & (PUNCH_N - 1u)] = (int16_t)clamp((l[i] + r[i]) >> 4, -32768, 32767);
        return;
    }
    mods = punch_mods();
    punch.mods = (uint8_t)mods;
    rate = (mods & (PM_SLOW | PM_FAST)) == PM_SLOW ? 32768u :
           (mods & (PM_SLOW | PM_FAST)) == PM_FAST ? 131072u : 65536u;
    strength = (mods & (PM_SOFT | PM_HARD)) == PM_SOFT ? -1 :
               (mods & (PM_SOFT | PM_HARD)) == PM_HARD ? 1 : 0;
    wet = strength < 0 ? 16384u : 32767u;
    if (mods & PM_BLEND) wet >>= 1;
    depth = strength < 0 ? 90 : strength > 0 ? 360 : 180;
    crush = strength < 0 ? 0x1ff : strength > 0 ? 0x1fff : 0x7ff;
    down = strength < 0 ? 4 : strength > 0 ? 16 : 8;
    feedback = strength < 0 ? 12000 : strength > 0 ? 24000 : 18000;
    wobble_inc = (4u * (0xFFFFFFFFu / FS) >> 2) * (rate >> 14);
    if (mods & PM_TRIP) wobble_inc += wobble_inc >> 1;
    if (punch.cur < 0 && want >= 0) {
        punch_start(want);
        punch.retrigger = 0;
    }
    punch.len = mods & PM_TRIP ? punch.base_len * 2u / 3u : punch.base_len;
    if (punch.k >= punch.len) punch.k %= punch.len;
    punch.dspd = (65536u / punch.len + 1u) * (rate >> 15) / 2u;
    beat = beat_samples();                              /* per block: no divide per sample */
    step = punch_division(beat / 4u, mods);
    echo_d = punch_division(beat * 3u / 4u, mods);
    if (echo_d > PUNCH_N - 64u) echo_d = PUNCH_N - 64u;
    if (punch.cur == PX_GATE)
        punch.gp = (song.playing ? clk_samples() : punch.t) % step;
    {   /* filter sweeps: per block */
        int32_t fx = punch.cur;
        if (fx == PX_LPF && punch.g > 0)
            punch.cut = clamp(punch.cut - (int32_t)(96u * (rate >> 15) / 2u),
                              (strength < 0 ? 64 : strength > 0 ? 24 : 34) << 8, 127 << 8);   /* closes over ~1 s */
        if (fx == PX_HPF && punch.g > 0)
            punch.cut = clamp(punch.cut - (int32_t)(64u * (rate >> 15) / 2u),
                              (strength < 0 ? 104 : strength > 0 ? 64 : 88) << 8, 127 << 8);
        tsvf_coef(&c1, fx == PX_LPF ? punch.cut : fx == PX_HPF ? (127 << 8) - punch.cut + (40 << 8) : 96 << 8, 90);
        tsvf_coef(&c2, 58 << 8, 40);                  /* PHONE: low cut ~ 500 Hz */
    }
    for (i = 0; i < n; i++) {
        int32_t x = l[i], y = r[i], wl = x, wr = y, m = (x + y) >> 1;
        int32_t fx = punch.cur;
        uint32_t target = want == fx && !punch.retrigger ? wet : 0u;
        if (!punch_owns(fx, punch.w))                   /* (a held loop keeps its material) */
            punch_ring[punch.w & (PUNCH_N - 1u)] = (int16_t)clamp(m >> 3, -32768, 32767);
        switch (fx) {
        case PX_LPF:
            wl = tsvf_lp(&c1, x >> 1, &punch.f1l, &punch.f2l) << 1;
            wr = tsvf_lp(&c1, y >> 1, &punch.f1r, &punch.f2r) << 1;
            break;
        case PX_HPF:
            wl = x - (tsvf_lp(&c1, x >> 1, &punch.f1l, &punch.f2l) << 1);
            wr = y - (tsvf_lp(&c1, y >> 1, &punch.f1r, &punch.f2r) << 1);
            break;
        case PX_TEL: {
            int32_t b = tsvf_lp(&c1, m >> 1, &punch.f1l, &punch.f2l);        /* < 2.5 kHz */
            b -= tsvf_lp(&c2, b, &punch.f3l, &punch.f4l);                   /* > 500 Hz */
            wl = wr = softclip(b << 2) >> 1;
            break;
        }
        case PX_CRUSH:
            wl = x >= 0 ? x & ~crush : -((-x) & ~crush);   /* towards zero: no offset */
            wr = y >= 0 ? y & ~crush : -((-y) & ~crush);
            break;
        case PX_DOWN:
            if (--punch.hn <= 0) {
                punch.hn = down;
                punch.held_l = x;
                punch.held_r = y;
            }
            wl = punch.held_l;
            wr = punch.held_r;
            break;
        case PX_GATE: {
            uint32_t p = punch.gp;
            uint32_t open = strength < 0 ? step * 3u / 4u : strength > 0 ? step / 4u : step / 2u;
            int32_t gg = p < open ? 32767 : 0;
            punch.gp = p + 1u >= step ? 0u : p + 1u;
            if (p < 64u) gg = (int32_t)p * 512;
            else if (p >= open && p < open + 64u) gg = 32767 - (int32_t)(p - open) * 512;
            wl = ((x >> 3) * gg) >> 12;
            wr = ((y >> 3) * gg) >> 12;
            break;
        }
        case PX_ECHO: {
            uint32_t d = echo_d;
            int32_t e;
            e = (ring_raw(punch.w - d) * feedback) >> 12;  /* (x 8 after: no overflow on a hot mix) */
            punch_ring[punch.w & (PUNCH_N - 1u)] = (int16_t)clamp((m + e) >> 3, -32768, 32767);   /* feedback */
            wl = x + e;
            wr = y + e;
            break;
        }
        default:
            if (fx < 0)
                break;
            if (punch.pre) {                            /* waiting for the division */
                if (!--punch.pre) {
                    punch.start = punch.w + 1u;
                    punch.k = 0;
                }
                break;
            }
            wl = wr = punch_ring_fx(fx, rate, wobble_inc, depth);
            break;
        }
        if ((uint32_t)punch.g < target)
            punch.g = punch.g + 512 > (int32_t)target ? (int32_t)target : punch.g + 512;
        else if ((uint32_t)punch.g > target)
            punch.g = punch.g < (int32_t)target + 512 ? (int32_t)target : punch.g - 512;
        l[i] = x + ((((wl - x) >> 3) * (punch.g >> 3)) >> 9);   /* (no 32-bit overflow) */
        r[i] = y + ((((wr - y) >> 3) * (punch.g >> 3)) >> 9);
        punch.w++;
        punch.t++;
        if (punch.g == 0 && (want != fx || punch.retrigger)) {                /* faded out: the next one, or none */
            if (want >= 0) punch_start(want);
            else punch.cur = -1;
            punch.retrigger = 0;
        }
    }
}

/* the keyboard (seq.c keyboard_block) while FX is held: white key index 0..15, -1 = black */
static int32_t punch_key(uint32_t k)
{
    static const int8_t W[12] = {0, -1, 1, -1, 2, -1, 3, 4, -1, 5, -1, 6};   /* from F */
    int32_t i = W[k % 12u];
    return i < 0 ? -1 : (int32_t)(k / 12u) * 7 + i;
}
