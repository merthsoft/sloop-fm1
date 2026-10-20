/* SPDX-License-Identifier: GPL-3.0-only */
/* Persistent record colour on the master bus. Original fixed-point implementation:
 * Slow record wow plus flutter (shared stereo delay), darkening and crackle.
 * All controls zero is bit-exact bypass. No allocation or sample-rate conversion. */
static int punch_vinyl_requested(void);
static struct {
    int32_t ring[2][256], low[2], wet, wow, tone, crackle, bed, click, rnd;
    uint32_t w, phase;
} vinyl;

static void vinyl_process(int32_t *l, int32_t *r, uint32_t n)
{
    int32_t wow = song.g[G_VWOW], tone = song.g[G_VTONE], crackle = song.g[G_VCRACKLE];
    if (punch_vinyl_requested()) { wow=96; tone=90; crackle=100; }
    if (!(wow | tone | crackle | vinyl.wet | vinyl.wow | vinyl.tone | vinyl.crackle)) {
        /* Clear history once on exit, so enabling it never replays an old song. */
        if (vinyl.w) {
            memset(vinyl.ring, 0, sizeof vinyl.ring);
            vinyl.w = 0; vinyl.low[0] = vinyl.low[1] = 0;
        }
        return;
    }
    if (!vinyl.rnd) vinyl.rnd = 0x3156494E;
    uint32_t phase = vinyl.phase;
    /* Up to ~24 cents of combined wow/flutter, within the existing 256-frame
     * ring. Smooth depth changes as well as the bypass crossfade. */
    int32_t depth1 = vinyl.wow + clamp(wow * 256 - vinyl.wow, -(int32_t)n * 32, (int32_t)n * 32);
    int32_t d0 = (96 << 16) + (sine_i(phase) * 96 + sine_i(phase * 8u) * 32) * (vinyl.wow >> 8) / 127;
    vinyl.phase += (uint32_t)(((uint64_t)0x100000000ull * 5u / (9u * FS)) * n);
    int32_t d1 = (96 << 16) + (sine_i(vinyl.phase) * 96 + sine_i(vinyl.phase * 8u) * 32) * (depth1 >> 8) / 127;
    int32_t dd = n ? (d1 - d0) / (int32_t)n : 0;
    for (uint32_t i = 0; i < n; i++, d0 += dd) {
        int32_t target = wow || vinyl.wow ? 32768 : 0;
        vinyl.wow += clamp(wow * 256 - vinyl.wow, -32, 32);
        vinyl.wet += clamp(target - vinyl.wet, -128, 128);
        vinyl.tone += clamp(tone * 256 - vinyl.tone, -32, 32);
        vinyl.crackle += clamp(crackle * 256 - vinyl.crackle, -32, 32);
        vinyl.bed += clamp((song.playing ? 32768 : 0) - vinyl.bed, -8, 8);
        uint32_t nz = noise32(&vinyl.rnd), w = vinyl.w++ & 255u;
        uint32_t pos = (w << 16) - (uint32_t)d0, a = (pos >> 16) & 255u;
        int32_t amount = vinyl.crackle >> 8;
        if ((nz >> 10) < (uint32_t)amount * 16u)
            vinyl.click = ((int32_t)(nz & 1023u) - 512) * amount / 32;
        vinyl.click -= vinyl.click / 4;
        int32_t noise = mulq16((vinyl.click + (((int32_t)(nz >> 16) - 32768) * amount >> 14)) * 8, vinyl.bed);
        for (uint32_t c = 0; c < 2u; c++) {
            int32_t *out = c ? r : l, x = out[i];
            vinyl.ring[c][w] = x;
            int32_t y = vinyl.ring[c][a] + mulq16(vinyl.ring[c][(a + 1u) & 255u] - vinyl.ring[c][a], pos & 65535u);
            x += mulq16(y - x, vinyl.wet * 2);
            /* Full headroom multiply: the master may exceed signed 16-bit. */
            vinyl.low[c] += mulq16(x - vinyl.low[c], (32767 - (vinyl.tone >> 8) * 232) * 2);
            out[i] = vinyl.tone ? vinyl.low[c] + noise : x + noise;
        }
    }
}
