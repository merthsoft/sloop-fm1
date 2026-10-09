/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP 2.4: the full-screen visualiser. On HOME (the TRACKS screen), HOME tapped opens it; HOME again or
 * any page closes it; SELECT steps through the fourteen styles (the name shows a second); a layer held shows
 * its screen as ever, then the visualiser comes back. The keys, PLAY and REC work as always.
 *   1 OSCILLOSCOPE  2 SPECTRUM  3 SPECTROGRAM  4 LISSAJOUS  5 VU METERS  6 CIRCLE
 *   7 ORBIT  8 WIRES  9 POLYRHYTHM  10 NOTE TRAILS  11 GROOVE
 *   12 STEREO FIELD  13 SONG JOURNEY  14 BEAT TERRAIN
 * It reads what the audio already leaves for the UI: the scope buffers (audio.c, the mix at 22 kHz, left and
 * right, as if MASTER were all the way up: the picture does not follow the volume knob, even at 0), the tracks' peaks (fx.c / drums.c, as the TRACKS meters read them), the notes that started
 * (voice.c vis_hit) and the transport clock. Everything is drawn in the main loop, every other frame, in two
 * bands of 120 rows (the canvas holds 124): no cost to the audio. */
#define VIS_N VIS_STYLE_COUNT
static const char *const VIS_NAME[VIS_N] = {"OSCILLOSCOPE", "SPECTRUM", "SPECTROGRAM", "LISSAJOUS", "VU METERS",
    "CIRCLE", "ORBIT", "WIRES", "POLYRHYTHM", "NOTE TRAILS", "GROOVE", "STEREO FIELD", "SONG JOURNEY", "BEAT TERRAIN"};
/* vis_on, vis_shown_last, vis_name_t: ui_draw.c; vis_style: panel.c (kept with the settings) */
#define VIS_FFT 512u
#define VIS_SG_W 48u                                /* spectrogram: 48 columns of 5 px, 72 rows of 3 px */
#define VIS_SG_H 72u
static int16_t vis_re[VIS_FFT] __attribute__((section(".pool"))), vis_im[VIS_FFT] __attribute__((section(".pool")));
/* Waveform snapshots and note trails are used by mutually exclusive styles. */
static union {
    struct { int16_t l[VIS_FFT], r[VIS_FFT]; } scope;
    struct { uint32_t notes[48][3][4]; uint16_t drums[48]; } trails;
} vis_frame;
#define vis_l vis_frame.scope.l
#define vis_r vis_frame.scope.r
#define vis_notes vis_frame.trails.notes
#define vis_drum_notes vis_frame.trails.drums
static uint8_t vis_frame_style = 255;
static uint8_t vis_sg[VIS_SG_H][VIS_SG_W] __attribute__((section(".pool")));
static uint8_t vis_lj[3][256][2];                   /* Lissajous: the last three frames' points */
static uint8_t vis_notes_head;
static struct {
    int32_t scale, ljscale, peak;                   /* auto-scales (smoothed peaks), this frame's peak */
    int16_t bar[32], cap[32], capv[32];             /* spectrum: bars, caps and their fall speed (0..1000) */
    int16_t vu[5], pk[5], pkt[5];                   /* VU: tracks + master, peak holds */
    int16_t lvl[5];                                 /* the tracks' levels (0..1000) this frame, the master */
    uint8_t flash[4], kick;                         /* frames since a note started (0 = none) / a kick, fading */
    int16_t wamp[4];                                /* WIRES: each string's swing (0..1000) */
    uint8_t wn[4];                                  /* its wave count (from the note) */
    uint32_t free_q8, last_ms, wph;                 /* beats (1/256) when stopped; the wires' phase */
    uint8_t lj_n;
} vs;
/* the spectrum's 32 bands: bins of the 512-point FFT at 22 050 Hz (43 Hz each), 40 Hz .. 10 kHz */
static const uint8_t VIS_EDGE[33] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 25, 29, 35,
                                     41, 49, 58, 69, 82, 98, 116, 138, 164, 195, 232};

static int vis_shown(void) { return vis_on && !ui.home && !ui.menu && cur_page()->scope == SC_TRK; }
static void vis_open(void)
{
    vis_on = 1;
    vis_name_t = 30;
    ui.force = 1;
}
static int32_t isin(uint32_t a) { return SINE[a & 1023u]; }          /* a: 1024 a turn; Q15 */
static int32_t icos(uint32_t a) { return SINE[(a + 256u) & 1023u]; }
static uint16_t mix565(uint16_t a, uint16_t b, int32_t k) /* share UI colour blending code */
{
    int32_t r = (a >> 11) + ((((b >> 11) - (a >> 11)) * k) >> 8);
    int32_t g = ((a >> 5) & 63) + (((((b >> 5) & 63) - ((a >> 5) & 63)) * k) >> 8);
    int32_t bl = (a & 31) + ((((b & 31) - (a & 31)) * k) >> 8);
    return (uint16_t)(r << 11 | g << 5 | bl);
}
/* |x| (Q15) -> 0..1000 over 48 dB (8 log2: 6 dB a step of 8) */
static int32_t vis_lvl(int32_t a)
{
    int32_t lg = 0, v;
    if (a < 0)
        a = -a;
    if (a < 16)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);           /* 120 = 0 dB */
    return clamp((v - 56) * 1000 / 64, 0, 1000);
}

/* ---- drawing helpers (canvas coordinates; cv_oy picks the band) ---- */
static void vis_ellipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
{
    int32_t y;
    if (rx < 1 || ry < 1)
        return;
    for (y = -ry; y <= ry; y++) {
        int32_t w = 0;
        while (w <= rx && (w * w) * (ry * ry) + (y * y) * (rx * rx) <= (rx * rx) * (ry * ry))
            w++;
        if (w)
            cv_rect(cx - w + 1, cy + y, 2 * w - 1, 1, c);
    }
}
static void vis_circle(int32_t cx, int32_t cy, int32_t r, uint16_t c) /* an outline */
{
    int32_t x = r, y = 0, e = 1 - r;
    while (x >= y) {
        cv_pset(cx + x, cy + y, c), cv_pset(cx - x, cy + y, c), cv_pset(cx + x, cy - y, c), cv_pset(cx - x, cy - y, c);
        cv_pset(cx + y, cy + x, c), cv_pset(cx - y, cy + x, c), cv_pset(cx + y, cy - x, c), cv_pset(cx - y, cy - x, c);
        y++;
        if (e < 0)
            e += 2 * y + 1;
        else
            e += 2 * (y - --x) + 1;
    }
}
static void vis_thick(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)   /* 2 px */
{
    cv_line(x0, y0, x1, y1, c);
    cv_line(x0, y0 + 1, x1, y1 + 1, c);
}
/* a polygon, points in 1/16 px (n <= 8), filled by scan lines (even-odd) inside the band */
static void vis_poly(const int32_t *px, const int32_t *py, uint32_t n, uint16_t c)
{
    int32_t y0 = 0x7FFFFFFF, y1 = -0x7FFFFFFF, y;
    uint32_t i;
    for (i = 0; i < n; i++) {
        if (py[i] < y0) y0 = py[i];
        if (py[i] > y1) y1 = py[i];
    }
    y0 = y0 >> 4;
    y1 = (y1 + 15) >> 4;
    if (y0 < -cv_oy) y0 = -cv_oy;
    if (y1 > (int32_t)cv_h - cv_oy) y1 = (int32_t)cv_h - cv_oy;
    for (y = y0; y < y1; y++) {
        int32_t xs[8], m = 0, sy = y * 16 + 8, a, b;
        for (i = 0; i < n && m < 8; i++) {
            uint32_t j = (i + 1u) % n;
            if ((py[i] <= sy && py[j] > sy) || (py[j] <= sy && py[i] > sy))
                xs[m++] = px[i] + (sy - py[i]) * (px[j] - px[i]) / (py[j] - py[i]);
        }
        for (a = 1; a < m; a++)                     /* (a few points: insertion sort) */
            for (b = a; b > 0 && xs[b - 1] > xs[b]; b--) {
                int32_t t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t;
            }
        for (a = 0; a + 1 < m; a += 2)
            cv_rect((xs[a] + 8) >> 4, y, ((xs[a + 1] + 8) >> 4) - ((xs[a] + 8) >> 4), 1, c);
    }
}
/* a seven-segment digit, the unlit segments faintly there (an LCD) */
static void vis_seg(int32_t x, int32_t y, int32_t w, int32_t h, int32_t t, int ch, uint16_t c)
{
    static const uint8_t SEG[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};   /* gfedcba */
    uint32_t m = ch >= 0 && ch <= 9 ? SEG[ch] : 0u;
    int32_t hh = h / 2;
    uint16_t off = RGB(18, 18, 21);
    cv_rect(x + t, y, w - 2 * t, t, m & 0x01 ? c : off);
    cv_rect(x + w - t, y + t, t, hh - t, m & 0x02 ? c : off);
    cv_rect(x + w - t, y + hh, t, hh - t, m & 0x04 ? c : off);
    cv_rect(x + t, y + h - t, w - 2 * t, t, m & 0x08 ? c : off);
    cv_rect(x, y + hh, t, hh - t, m & 0x10 ? c : off);
    cv_rect(x, y + t, t, hh - t, m & 0x20 ? c : off);
    cv_rect(x + t, y + hh - t / 2, w - 2 * t, t, m & 0x40 ? c : off);
}

/* ---- the frame's data ---- */
/* beats since PLAY in 1/256 (stopped: a free count at the tempo, for what moves anyway) */
static uint32_t vis_beat_q8(void)
{
    if (song.playing)
        return clk_beat * 256u + clk_pos / (BEAT_U / 256u);
    return vs.free_q8;
}
static void vis_fft(void)
{
    uint32_t i, j = 0, bit, len;
    for (i = 0; i < VIS_FFT; i++) {                 /* Hann window */
        int32_t w = (32768 - icos(i * 2u)) >> 1;
        vis_re[i] = (int16_t)((vis_l[i] * w) >> 15);
        vis_im[i] = 0;
    }
    for (i = 1; i < VIS_FFT; i++) {                 /* bit reversal */
        for (bit = VIS_FFT >> 1; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            int16_t t = vis_re[i]; vis_re[i] = vis_re[j]; vis_re[j] = t;
        }
    }
    for (len = 2; len <= VIS_FFT; len <<= 1) {      /* radix 2, halved each stage (no overflow) */
        uint32_t half = len >> 1, k;
        for (i = 0; i < VIS_FFT; i += len)
            for (k = 0; k < half; k++) {
                uint32_t a = i + k, b = a + half, ix = k * (1024u / len);
                int32_t wr = icos(ix), wi = -isin(ix);
                int32_t tr = (vis_re[b] * wr - vis_im[b] * wi) >> 15, ti = (vis_re[b] * wi + vis_im[b] * wr) >> 15;
                int32_t ar = vis_re[a], ai = vis_im[a];
                vis_re[b] = (int16_t)((ar - tr) >> 1);
                vis_im[b] = (int16_t)((ai - ti) >> 1);
                vis_re[a] = (int16_t)((ar + tr) >> 1);
                vis_im[a] = (int16_t)((ai + ti) >> 1);
            }
    }
}
/* the 32 bands of the last FFT, 0..1000 (60 dB under a full-scale sine) */
static void vis_bands(int16_t *out)
{
    uint32_t k, i;
    for (k = 0; k < 32u; k++) {
        int32_t m = 0, lg = 0, v;
        for (i = VIS_EDGE[k]; i < VIS_EDGE[k + 1] && i < VIS_FFT / 2u; i++) {
            int32_t a = vis_re[i] < 0 ? -vis_re[i] : vis_re[i], b = vis_im[i] < 0 ? -vis_im[i] : vis_im[i];
            int32_t mag = a > b ? a + b / 2 : b + a / 2;
            if (mag > m)
                m = mag;
        }
        if (m < 2) {
            out[k] = 0;
            continue;
        }
        while ((m >> lg) > 1)
            lg++;
        v = lg * 8 + (((m << 3) >> lg) & 7);        /* a full-scale sine: ~104 */
        out[k] = (int16_t)clamp((v - 24) * 1000 / 80, 0, 1000);
    }
}

static void vis_update(void)
{
    uint32_t i, w, hit, now = fm1_ms;
    int32_t pk[4], mpk = 0;
    if (vis_frame_style != vis_style) {
        if (vis_style == 9u) memset(&vis_frame.trails, 0, sizeof vis_frame.trails);
        vis_frame_style = vis_style;
    }
    /* the scope: the last 512 frames (left, right) */
    w = scope_w;
    for (i = 0; i < VIS_FFT; i++) {
        int16_t sample = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (vis_style != 9u) {
            vis_l[i] = sample;
            vis_r[i] = scope_bufr[(w + i) & (SCOPE_N - 1u)];
        }
        if (sample > mpk) mpk = sample;
        if (-sample > mpk) mpk = -sample;
    }
    /* the tracks' peaks since the last look (as the TRACKS meters take them) and the notes that started */
    fm1_irq_off();
    for (i = 0; i < NPART; i++) {
        pk[i] = trk[i].peak;
        trk[i].peak = 0;
    }
    pk[3] = drums.peak;
    drums.peak = 0;
    hit = vis_hit;
    vis_hit = 0;
    if (vis_kick_hit) {
        vis_kick_hit = 0;
        vs.kick = 255;
    }
    fm1_irq_on();
    for (i = 0; i < 4u; i++) {
        int32_t v = trk_level(i) && !trk[i].p[P_MUTE] ? vis_lvl(pk[i]) : 0;   /* (as the TRACKS meters) */
        vs.lvl[i] = (int16_t)v;
        if ((hit >> i) & 1u) {
            vs.flash[i] = 8;
            vs.wn[i] = (uint8_t)(2u + vis_note[i] % 12u / 2u);
            if (vs.wamp[i] < 1000) vs.wamp[i] = 1000;
        } else if (vs.flash[i]) {
            vs.flash[i]--;
        }
        vs.wamp[i] = (int16_t)(vs.wamp[i] > v ? vs.wamp[i] - (vs.wamp[i] - v) / 6 - 4 : v);
        if (vs.wamp[i] < 0) vs.wamp[i] = 0;
    }
    vs.lvl[4] = (int16_t)vis_lvl(mpk);
    vs.kick = (uint8_t)(vs.kick > 20 ? vs.kick - 20 : 0);
    /* the free beat count (stopped) */
    {
        uint32_t dt = now - vs.last_ms;
        vs.last_ms = now;
        if (dt > 200u) dt = 200u;
        vs.free_q8 += dt * (uint32_t)song.g[G_BPM] * 256u / 60000u;
    }
    vs.wph += 37u;
    vs.peak = mpk;
    if (vis_style == 9u) {
        uint32_t t, v;
        vis_notes_head = (uint8_t)((vis_notes_head + 1u) % 48u);
        memset(vis_notes[vis_notes_head], 0, sizeof vis_notes[0]);
        fm1_irq_off();
        for (t = 0; t < NPART; t++)
            for (v = 0; v < NVOICE; v++) {
                const voice_t *n = &trk[t].v[v];
                if (n->active && n->note < 128u && !trk_silent(&trk[t]))
                    vis_notes[vis_notes_head][t][n->note >> 5] |= 1u << (n->note & 31u);
            }
        fm1_irq_on();
        vis_drum_notes[vis_notes_head] = 0;
        for (t = 0; t < 16u; t++) if (pad_lit[t]) vis_drum_notes[vis_notes_head] |= (uint16_t)(1u << t);
    }
    {                                               /* the wave's scale follows the peak */
        int32_t target = mpk < 1600 ? 1600 : mpk;
        vs.scale += (target - vs.scale) / 5;
    }
    switch (vis_style) {
    case 1: case 2: case 13: {
        int16_t b[32];
        vis_fft();
        vis_bands(b);
        if (vis_style != 2) {
            for (i = 0; i < 32u; i++) {
                int32_t bar = vs.bar[i] - vs.bar[i] * 18 / 100;
                vs.bar[i] = (int16_t)(b[i] > bar ? b[i] : bar);
                if (vs.bar[i] >= vs.cap[i]) {
                    vs.cap[i] = vs.bar[i];
                    vs.capv[i] = 0;
                } else {
                    vs.capv[i] = (int16_t)(vs.capv[i] + 4);
                    vs.cap[i] = (int16_t)(vs.cap[i] > vs.capv[i] ? vs.cap[i] - vs.capv[i] : 0);
                }
            }
        } else {                                    /* spectrogram: the new row on top */
            for (i = VIS_SG_H - 1u; i; i--)            /* (no memmove here: row by row, from the bottom) */
                memcpy(vis_sg[i], vis_sg[i - 1u], VIS_SG_W);
            for (i = 0; i < VIS_SG_W; i++) {
                uint32_t p = i * 31u * 16u / (VIS_SG_W - 1u), k = p >> 4, f = p & 15u;
                int32_t v = k < 31u ? (b[k] * (int32_t)(16u - f) + b[k + 1u] * (int32_t)f) / 16 : b[31];
                vis_sg[0][i] = (uint8_t)(v * 255 / 1000);
            }
        }
        break;
    }
    case 3: {                                       /* Lissajous: L-R across, L+R up; three frames kept */
        int32_t m = 1;
        uint8_t (*p)[2];
        for (i = 0; i < VIS_FFT; i += 2u) {
            int32_t s = vis_l[i] + vis_r[i];
            if (s < 0) s = -s;
            if (s > m) m = s;
        }
        if (m < 3000) m = 3000;
        vs.ljscale += (m - vs.ljscale) / 5;
        if (vs.ljscale < 1) vs.ljscale = 1;
        vs.lj_n = (uint8_t)((vs.lj_n + 1u) % 3u);
        p = vis_lj[vs.lj_n];
        for (i = 0; i < 256u; i++) {
            int32_t l = vis_l[i * 2u], r = vis_r[i * 2u];
            p[i][0] = (uint8_t)clamp(120 + (l - r) * 170 / vs.ljscale, 0, 239);
            p[i][1] = (uint8_t)clamp(120 - (l + r) * 95 / vs.ljscale, 0, 239);
        }
        break;
    }
    case 4:                                         /* VU: fall 3 % a frame, peaks held a second */
        for (i = 0; i < 5u; i++) {
            int32_t v = vs.lvl[i];
            vs.vu[i] = (int16_t)(v > vs.vu[i] - 30 ? v : vs.vu[i] - 30);
            if (v >= vs.pk[i]) {
                vs.pk[i] = (int16_t)v;
                vs.pkt[i] = 0;
            } else if (++vs.pkt[i] > 30) {
                vs.pk[i] = (int16_t)(vs.pk[i] > 20 ? vs.pk[i] - 20 : 0);
            }
        }
        break;
    default:
        break;
    }
}

/* ---- the styles (each draws the whole screen; called once per band) ---- */
static void vis_scope(void)
{
    uint32_t i0 = 0, i;
    int32_t py = 0;
    for (i = 1; i + 240u < VIS_FFT; i++)            /* a rising zero crossing: the wave stands still */
        if (vis_l[i - 1u] < 0 && vis_l[i] >= 0) {
            i0 = i;
            break;
        }
    cv_rect(0, 120, 240, 1, C_LINE);
    for (i = 0; i < 240u; i += 40u)
        cv_rect((int32_t)i, 116, 1, 9, C_LINE);
    for (i = 0; i < 240u; i++) {
        int32_t y = 120 - vis_l[i0 + i] * 100 / (vs.scale ? vs.scale : 1);
        y = clamp(y, 2, 237);
        if (i) {
            cv_line((int32_t)i - 1, py - 1, (int32_t)i, y - 1, TE_DIM[0]);
            cv_line((int32_t)i - 1, py + 2, (int32_t)i, y + 2, TE_DIM[0]);
            vis_thick((int32_t)i - 1, py, (int32_t)i, y, TE_COL[0]);
        }
        py = y;
    }
}
static void vis_spectrum(void)
{
    uint32_t k;
    for (k = 0; k < 32u; k++) {
        int32_t x0 = 4 + (int32_t)k * 7, h = vs.bar[k] * 210 / 1000, y, cy = 232 - vs.cap[k] * 210 / 1000;
        for (y = 232; y > 232 - h; y -= 4)          /* LED segments */
            cv_rect(x0, y - 2, 5, 3, TE_COL[k / 8u]);
        cv_rect(x0, cy - 1, 5, 2, C_WHITE);
    }
}
static uint16_t vis_heat(uint32_t v)               /* 0..255: black, blue, green, yellow, white */
{
    static const uint16_t ST[5] = {RGB(0, 0, 0), RGB(40, 124, 255), RGB(30, 204, 112), RGB(255, 198, 24), RGB(255, 255, 255)};
    static const uint8_t AT[5] = {0, 77, 140, 204, 255};
    uint32_t k;
    v = (v + ((v * v) >> 8)) >> 1;                  /* (quiet parts a little darker: about v^1.4) */
    for (k = 1; k < 5u && v > AT[k]; k++)
        ;
    if (k >= 5u)
        return ST[4];
    return mix565(ST[k - 1u], ST[k], (int32_t)((v - AT[k - 1u]) * 256u / (AT[k] - AT[k - 1u])));
}
static void vis_spectrogram(void)
{
    uint32_t r, c;
    for (r = 0; r < VIS_SG_H; r++) {
        int32_t y = 24 + (int32_t)r * 3;
        if (y + 3 <= -cv_oy || y >= (int32_t)cv_h - cv_oy)
            continue;
        for (c = 0; c < VIS_SG_W; c++)
            if (vis_sg[r][c] > 8u)
                cv_rect((int32_t)c * 5, y, 5, 3, vis_heat(vis_sg[r][c]));
    }
    cv_text(2, 2, &FONT_S, "40", C_DIM);
    cv_text(30, 2, &FONT_S, "200", C_DIM);
    cv_text(132, 2, &FONT_S, "1k", C_DIM);
    cv_text(196, 2, &FONT_S, "4k", C_DIM);
}
static void vis_lissajous(void)
{
    uint32_t f, i;
    static const uint16_t COLS[3] = {RGB(10, 66, 38), RGB(20, 136, 76), RGB(30, 204, 112)};
    cv_rect(120, 30, 1, 180, C_LINE);
    cv_rect(30, 120, 180, 1, C_LINE);
    for (f = 0; f < 3u; f++) {                      /* the oldest first, dimmest */
        const uint8_t (*p)[2] = vis_lj[(vs.lj_n + 1u + f) % 3u];
        for (i = 0; i < 256u; i++)
            cv_pset(p[i][0], p[i][1], COLS[f]);
    }
    cv_text(116, 4, &FONT_S, "M", C_DIM);
    cv_text(6, 112, &FONT_S, "L", C_DIM);
    cv_text(226, 112, &FONT_S, "R", C_DIM);
}
static void vis_meters(void)
{
    uint32_t i, k;
    for (i = 0; i < 5u; i++) {
        int32_t x0 = 10 + (int32_t)i * 46, on = vs.vu[i] * 24 / 1000, pk = vs.pk[i] * 24 / 1000;
        uint16_t c = i < 4u ? TE_COL[i] : C_WHITE;
        char lab[2] = {(char)(i < 4u ? '1' + i : 'M'), 0};
        for (k = 0; k < 24u; k++) {
            int32_t y = 206 - (int32_t)k * 8;
            uint16_t col = (int32_t)k < on ? (k < 18u ? c : k < 22u ? TE_COL[2] : RGB(255, 44, 52)) : TE_G1;
            if ((int32_t)k == pk && pk > 0)
                col = C_WHITE;
            cv_rect(x0, y - 5, 35, 6, col);
        }
        cv_text(x0 + 13, 216, &FONT_S, lab, c);
    }
}
static void vis_ring(void)
{
    int32_t k = vs.kick, r0 = 52 + k * 18 / 255, rc = r0 * 6 / 10, sc = vs.peak > vs.scale ? vs.peak : vs.scale;
    int32_t px = 0, py = 0, i;
    vis_ellipse(120, 120, rc, rc, RGB(40 + 140 * k / 255, 14 + 50 * k / 255, 4));
    for (i = 0; i <= 180; i++) {                    /* the wave round the ring, 2 degrees a point */
        uint32_t a = (uint32_t)(i % 180) * 1024u / 180u, j = 20u + (uint32_t)(i % 180) * 2u;
        int32_t s = (vis_l[j - 2u] + vis_l[j - 1u] + vis_l[j] + vis_l[j + 1u] + vis_l[j + 2u]) / 5;
        int32_t rr = r0 + s * 20 / (sc > 1600 ? sc : 1600), x = 120 + ((icos(a) * rr) >> 15), y = 120 + ((isin(a) * rr) >> 15);
        if (i) {
            cv_line(px, py - 1, x, y - 1, RGB(120, 40, 10));
            cv_line(px, py + 2, x, y + 2, RGB(120, 40, 10));
            vis_thick(px, py, x, y, TE_COL[3]);
        }
        px = x, py = y;
    }
}
static void vis_orbit(void)
{
    static const uint8_t RAD[4] = {42, 62, 82, 102}, PER[4] = {1, 2, 4, 8};
    uint32_t bq = vis_beat_q8(), i, k;
    int32_t rs = 14 + vs.lvl[4] * 22 / 1000;
    for (i = 0; i < 4u; i++) {
        uint32_t a = (bq * 4u / PER[i]) - 256u;     /* a turn = 1024: per beats, from the top */
        int32_t pr = 4 + vs.lvl[i] * 8 / 1000;
        vis_circle(120, 120, RAD[i], RGB(36, 36, 42));
        for (k = 1; k < 7u; k++) {                  /* its trail */
            uint32_t ak = a - k * 13u;
            vis_ellipse(120 + ((icos(ak) * RAD[i]) >> 15), 120 + ((isin(ak) * RAD[i]) >> 15), 2, 2,
                        mix565(TE_COL[i], 0, (int32_t)(60u + k * 196u / 7u)));
        }
        vis_ellipse(120 + ((icos(a) * RAD[i]) >> 15), 120 + ((isin(a) * RAD[i]) >> 15), pr, pr, TE_COL[i]);
    }
    vis_ellipse(120, 120, rs, rs, C_WHITE);
}
static void vis_wires(void)
{
    uint32_t i, x;
    for (i = 0; i < 4u; i++) {
        int32_t y0 = 40 + (int32_t)i * 52, amp = vs.wamp[i] * 22 / 1000, py = y0;
        char lab[2] = {(char)('1' + i), 0};
        for (x = 0; x < 240u; x += 2u) {           /* a standing wave: the string's shape x its swing */
            uint32_t a = x * 512u / 240u, cyc = i == 3u ? 8u : vs.wn[i] ? vs.wn[i] : 2u;   /* sin(pi x / L) */
            int32_t env = isin(a), wv = isin(x * cyc * 1024u / 240u + vs.wph * (3u + i));
            int32_t y = y0 - (((env * wv) >> 15) * amp >> 15);
            if (x)
                vis_thick((int32_t)x - 2, py, (int32_t)x, y, TE_COL[i]);
            py = y;
        }
        vis_ellipse(4, y0, 4, 4, C_WHITE);
        vis_ellipse(236, y0, 4, 4, C_WHITE);
        cv_text(10, y0 - 22, &FONT_S, lab, TE_COL[i]);
    }
}
/* the logo (tools/gen_logo.py at 0.86, centred on 120, 112), alive: the dial is the master, the sail's four
 * bands the tracks (growing with their level, flashing on a note), the boat rocks on the master's wave */


/* Sequencer views use each track's own grid, including long divisions and swing. */
static uint32_t vis_position(const track_t *t, uint32_t *fraction)
{
    uint32_t into, len, at = trk_grid(t, &into, &len), unit = len / 256u;
    *fraction = clamp((int32_t)(into / (unit ? unit : 1u)), 0, 255);
    return at % trk_len(t);
}
static int vis_has(const track_t *t, uint32_t s)
{
    return is_drum(t) ? dstep_mask(&t->dstep[s]) != 0 : t->step[s].n && t->step[s].time != ST_REST;
}
static void vis_polyrhythm(void)
{
    uint32_t t, s;
    for (t = 0; t < 4u; t++) {
        uint32_t f, len = trk_len(&trk[t]), at = vis_position(&trk[t], &f);
        int32_t r = 34 + (int32_t)t * 22;
        vis_circle(120, 123, r, TE_DIM[t]);
        for (s = 0; s < len; s++) {
            uint32_t a = s * 1024u / len - 256u;
            int32_t x = 120 + icos(a) * r / 32768, y = 123 + isin(a) * r / 32768;
            vis_ellipse(x, y, vis_has(&trk[t], s) ? 3 : 1, vis_has(&trk[t], s) ? 3 : 1,
                        vis_has(&trk[t], s) ? TE_COL[t] : TE_DIM[t]);
        }
        { uint32_t a = (at * 256u + f) * 4u / len - 256u;
          vis_ellipse(120 + icos(a) * r / 32768, 123 + isin(a) * r / 32768, 4, 4, C_WHITE); }
    }
}
static void vis_trails(void)
{
    uint32_t i, t, n;
    for (n = 0; n < 128u; n += 12u)
        cv_rect(0, 216 - (int32_t)n * 3 / 2, 240, 1, C_LINE);
    for (i = 0; i < 48u; i++)
        for (t = 0; t < 3u; t++)
            for (n = 0; n < 128u; n++)
                if ((vis_notes[(vis_notes_head + 1u + i) % 48u][t][n >> 5] >> (n & 31u)) & 1u)
                    cv_rect((int32_t)i * 5, 216 - (int32_t)n * 3 / 2, 5, 2, TE_COL[t]);
    cv_rect(235, 24, 1, 194, C_WHITE);
    cv_rect(0, 223, 240, 16, TE_DIM[3]);
    for (i = 0; i < 48u; i++)
        for (n = 0; n < 16u; n++)
            if ((vis_drum_notes[(vis_notes_head + 1u + i) % 48u] >> n) & 1u)
                cv_rect((int32_t)i * 5, 223 + (int32_t)n, 5, 1, TE_COL[3]);
}
static void vis_groove(void)
{
    uint32_t t, s;
    cv_text(8, 24, &FONT_S, "GRID / SWING / NUDGE", C_DIM);
    for (t = 0; t < 4u; t++) {
        track_t *p = &trk[t];
        uint32_t f, at = vis_position(p, &f), base = at / 8u * 8u, len = trk_len(p);
        int32_t y = 65 + (int32_t)t * 48;
        cv_rect(8, y + 15, 224, 1, TE_DIM[t]);
        for (s = 0; s < 8u; s++) {
            uint32_t idx = (base + s) % len, k, rats = 1, velocity = 100;
            int32_t x = 14 + (int32_t)s * 28, shift = p->micro[idx] * 28 / 64;
            if ((idx & 1u) && swings(trk_div(p)))
                shift += clamp(p->p[P_SSWING] + song.g[G_SWING], 0, 100) * 28 / 200;
            cv_rect(x, y - 8, 1, 30, C_LINE);
            if (!vis_has(p, idx)) continue;
            if (is_drum(p)) {
                for (k = 0; k < 16u; k++) if (dstep_has(&p->dstep[idx], k)) {
                    uint32_t r = dstep_rat(&p->dstep[idx], k) + 1u;
                    velocity = dstep_lvl(&p->dstep[idx], k) == LV_GHOST ? 35u : dstep_lvl(&p->dstep[idx], k) == LV_SOFT ? 65u : 100u;
                    if (r > rats) rats = r;
                }
            } else { rats = (p->step[idx].rat & 3u) + 1u; velocity = p->step[idx].vel; }
            cv_line(x, y + 15, x + shift, y, TE_DIM[t]);
            for (k = 0; k < rats; k++)
                vis_ellipse(x + shift + (int32_t)k * 24 / (int32_t)rats, y, 2 + (int32_t)velocity / 50, 2 + (int32_t)velocity / 50,
                            idx == at ? C_WHITE : mix565(TE_DIM[t], TE_COL[t], (int32_t)velocity * 2));
        }
    }
}
static void vis_stereo(void)
{
    uint32_t i;
    int32_t lsum = 0, rsum = 0, diff = 0, total = 0, scale = vs.scale > 1600 ? vs.scale : 1600;
    cv_rect(120, 30, 1, 168, C_LINE); cv_rect(20, 120, 200, 1, C_LINE);
    for (i = 0; i < VIS_FFT; i += 4u) {
        int32_t l = vis_l[i], r = vis_r[i], d = r - l;
        lsum += l < 0 ? -l : l; rsum += r < 0 ? -r : r;
        diff += d < 0 ? -d : d; total += (l < 0 ? -l : l) + (r < 0 ? -r : r);
        vis_ellipse(clamp(120 + d * 90 / scale, 12, 228), clamp(120 - (l + r) * 65 / scale, 30, 195), 2, 2, vis_heat((uint32_t)vs.lvl[4] * 255u / 1000u));
    }
    cv_text(8, 204, &FONT_S, "L     BALANCE     R", C_DIM);
    cv_rect(20, 224, 200, 3, C_LINE);
    vis_ellipse(120 + (rsum - lsum) * 90 / (total ? total : 1), 225, 4, 4, C_WHITE);
    cv_rect(20, 234, clamp(diff * 200 / (total ? total : 1), 0, 200), 3, TE_COL[0]);
}
static void vis_journey(void)
{
    uint32_t i, n = chain_n ? chain_n : arrangement.count, at = chain_n ? chain_i : arrangement_clock.index;
    uint32_t bars = chain_n ? chain_bars : arrangement.entry[at % ARR_STEPS].bars;
    uint32_t elapsed = chain_n ? live_bar : arrangement_clock.bar;
    int running = song.playing && (chain_n || arrangement_clock.running);
    char lab[24];
    if (n > ARR_STEPS) n = ARR_STEPS;
    cv_text(14, 30, &FONT_S, chain_n ? "QUICK CHAIN" : "SONG ORDER", C_DIM);
    for (i = 0; i < n; i++) {
        uint32_t sec = chain_n ? chain_sec[i % CHAIN_MAX] & 3u : arrangement.entry[i].scene & 3u;
        int32_t x = 12 + (int32_t)(i % 4u) * 57, y = 65 + (int32_t)(i / 4u) * 32;
        char s[2] = {(char)('A' + sec), 0};
        cv_rect(x, y, 50, 25, i == at ? TE_COL[sec] : TE_DIM[sec]);
        cv_text(x + 20, y + 4, &FONT_S, s, i == at ? C_BLACK : C_WHITE);
    }
    if (running) {
        fmt_int(lab, (int32_t)(elapsed < bars ? bars - elapsed : 0));
        str_cpy(lab + str_len(lab), " BARS LEFT", 12);
    } else str_cpy(lab, "READY", sizeof lab);
    cv_text(14, 205, &FONT_S, lab, C_WHITE);
    cv_rect(12, 230, 216, 4, C_LINE);
    cv_rect(12, 230, (int32_t)(elapsed < bars ? elapsed : bars) * 216 / (int32_t)(bars ? bars : 1u), 4, TE_COL[0]);
}
static void vis_terrain(void)
{
    int32_t row, col, px[17], py[17], prevx[17], prevy[17];
    uint32_t beat = vis_beat_q8();
    cv_text(8, 25, &FONT_S, "BASS     MID     HIGH", C_DIM);
    for (row = 0; row < 12; row++) {
        int32_t depth = 12 + row * 8 + (int32_t)(beat & 255u) / 32, y = 65 + depth * depth / 65;
        for (col = 0; col <= 16; col++) {
            uint32_t band = (uint32_t)col * 31u / 16u;
            int32_t h = vs.bar[band] * depth / 2200;
            px[col] = 120 + (col - 8) * depth / 6;
            py[col] = y - h + isin((uint32_t)(col * 90 + row * 70) + beat) * h / 65536;
            if (col) cv_line(px[col - 1], py[col - 1], px[col], py[col], TE_COL[band / 8u]);
            if (row) cv_line(prevx[col], prevy[col], px[col], py[col], TE_DIM[band / 8u]);
            prevx[col] = px[col]; prevy[col] = py[col];
        }
    }
}

/* the visualiser, every other frame: the update, then both bands */
static void vis_draw(void)
{
    uint32_t pass;
    if (!vis_shown_last || ui.force) {
        vis_shown_last = 1;
        lcd_fill(0, 0, 240, 240, C_BLACK);
    } else if (ui.frame & 1u) {
        return;
    }
    vis_update();
    for (pass = 0; pass < 2u; pass++) {
        cv_begin(240, 120, C_BLACK);
        cv_oy = -(int32_t)(pass * 120u);
        switch (vis_style % VIS_N) {
        case 0: vis_scope(); break;
        case 1: vis_spectrum(); break;
        case 2: vis_spectrogram(); break;
        case 3: vis_lissajous(); break;
        case 4: vis_meters(); break;
        case 5: vis_ring(); break;
        case 6: vis_orbit(); break;
        case 7: vis_wires(); break;
        case 8: vis_polyrhythm(); break;
        case 9: vis_trails(); break;
        case 10: vis_groove(); break;
        case 11: vis_stereo(); break;
        case 12: vis_journey(); break;
        case 13: vis_terrain(); break;
        default: vis_scope(); break;
        }
        if (!pass && (vis_name_t || ui.msg_t)) {    /* the style's name a second (or a message) */
            char n[8];
            const char *s = ui.msg_t ? ui.msg : VIS_NAME[vis_style % VIS_N];
            cv_rect(0, 0, 240, 20, C_BLACK);
            cv_text(120 - text_w(&FONT_S, s) / 2, 2, &FONT_S, s, C_WHITE);
            if (!ui.msg_t) {
                fmt_int(n, (int32_t)(vis_style % VIS_N) + 1);
                str_cpy(n + str_len(n), "/14", 4);
                cv_text(4, 2, &FONT_S, n, C_DIM);
            }
        }
        cv_oy = 0;
        cv_blit(0, pass * 120u);
    }
    if (vis_name_t)
        vis_name_t--;
}
/* SELECT on the visualiser: the next / previous style (round) */
static void vis_select(int32_t s)
{
    vis_style = (uint8_t)((vis_style + (s > 0 ? 1u : VIS_N - 1u)) % VIS_N);
    vis_name_t = 30;
    ui.force = 1;
    settings_later = 1;                                 /* (kept with the settings, once stopped) */
}
