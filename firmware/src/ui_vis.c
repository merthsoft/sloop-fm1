/* SPDX-License-Identifier: GPL-3.0-only */
/* SLOOP 2.4: the full-screen visualiser. On HOME (the TRACKS screen), HOME tapped opens it; HOME again or
 * any page closes it; SELECT steps through the seven styles (the name shows a second); a layer held shows
 * its screen as ever, then the visualiser comes back. The keys, PLAY and REC work as always.
 *   1 OSCILLOSCOPE  2 SPECTRUM  3 SPECTROGRAM  4 VU METERS
 *   5 NOTE TRAILS  6 GROOVE  7 SONG JOURNEY
 * It reads what the audio already leaves for the UI: the scope buffers (audio.c, the mix at 22 kHz, left and
 * right, as if MASTER were all the way up: the picture does not follow the volume knob, even at 0), the tracks' peaks (fx.c / drums.c, as the TRACKS meters read them), the notes that started
 * (voice.c note_hits) and the transport clock. Everything is drawn in the main loop, every other frame, in two
 * bands of 120 rows (the canvas holds 124): no per-sample visualizer work. */
#define VIS_N VIS_STYLE_COUNT
static const char *const VIS_NAME[VIS_N] = {"OSCILLOSCOPE", "SPECTRUM", "SPECTROGRAM", "VU METERS",
    "NOTE TRAILS", "GROOVE", "SONG JOURNEY"};
/* vis_on, vis_shown_last, vis_name_t: ui_draw.c; vis_style: panel.c (kept with the settings) */
#define VIS_FFT 512u
#define VIS_SG_W 48u                                /* spectrogram: 48 columns of 5 px, 72 rows of 3 px */
#define VIS_SG_H 72u
_Static_assert(SCOPE_N == VIS_FFT, "shared scope/FFT snapshot size");
#define vis_re ui_scope_snap
static int16_t vis_im[VIS_FFT] __attribute__((section(".pool")));
/* Waveform and trail histories share storage; clear trails on entry. */
static union {
    struct { int16_t l[VIS_FFT]; } scope;
    struct { uint32_t notes[48][3][4]; uint16_t drums[48]; } trails;
} vis_frame __attribute__((section(".scratch")));
#define vis_l vis_frame.scope.l
#define vis_notes vis_frame.trails.notes
#define vis_drum_notes vis_frame.trails.drums
static uint8_t vis_frame_style = 255;
static uint8_t vis_sg[VIS_SG_H][VIS_SG_W] __attribute__((section(".pool")));
static uint8_t vis_notes_head;
static struct {
    int32_t scale;                                /* smoothed waveform peak */
    int16_t bar[32], cap[32], capv[32];             /* spectrum: bars, caps and their fall speed (0..1000) */
    int16_t vu[5], pk[5], pkt[5];                   /* VU: tracks + master, peak holds */
    int16_t lvl[5];                                 /* the tracks' levels (0..1000) this frame, the master */
} vs;
/* the spectrum's 32 bands: bins of the 512-point FFT at 22 050 Hz (43 Hz each), 40 Hz .. 10 kHz */
static const uint8_t VIS_EDGE[33] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 25, 29, 35,
                                     41, 49, 58, 69, 82, 98, 116, 138, 164, 195, 232};

UI_SMALL static int vis_shown(void) { return vis_on && !ui.home && !ui.menu && cur_page()->scope == SC_TRK; }
UI_SMALL static void vis_open(void)
{
    vis_on = 1;
    vis_name_t = 30;
    ui.force = 1;
}
UI_SMALL static int32_t isin(uint32_t a) { return SINE[a & 1023u]; }          /* a: 1024 a turn; Q15 */
UI_SMALL static int32_t icos(uint32_t a) { return SINE[(a + 256u) & 1023u]; }
UI_SMALL static uint16_t mix565(uint16_t a, uint16_t b, int32_t k) /* share UI colour blending code */
{
    int32_t r = (a >> 11) + ((((b >> 11) - (a >> 11)) * k) >> 8);
    int32_t g = ((a >> 5) & 63) + (((((b >> 5) & 63) - ((a >> 5) & 63)) * k) >> 8);
    int32_t bl = (a & 31) + ((((b & 31) - (a & 31)) * k) >> 8);
    return (uint16_t)(r << 11 | g << 5 | bl);
}
/* |x| (Q15) -> 0..1000 over 48 dB (8 log2: 6 dB a step of 8) */
UI_SMALL static int32_t vis_lvl(int32_t a)
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
UI_SMALL static void vis_ellipse(int32_t cx, int32_t cy, int32_t rx, int32_t ry, uint16_t c)
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
UI_SMALL static void vis_thick(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)   /* 2 px */
{
    cv_line(x0, y0, x1, y1, c);
    cv_line(x0, y0 + 1, x1, y1 + 1, c);
}
/* ---- the frame's data ---- */
UI_SMALL static void vis_fft(void)
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
UI_SMALL static void vis_bands(int16_t *out)
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

/* Keeping frame preparation separate reduces target UI code size. */
UI_SMALL static void vis_update(void)
{
    uint32_t i, w;
    int32_t pk[4], mpk = 0;
    if (vis_frame_style != vis_style) {
        if (vis_style == VIS_TRAILS) memset(&vis_frame.trails, 0, sizeof vis_frame.trails);
        vis_frame_style = vis_style;
    }
    /* The remaining waveform analyzers consume only the left mix. */
    w = scope_w;
    for (i = 0; i < VIS_FFT; i++) {
        int16_t sample = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (vis_style != VIS_TRAILS) {
            vis_l[i] = sample;
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
    fm1_irq_on();
    for (i = 0; i < 4u; i++) {
        int32_t v = trk_level(i) && !trk[i].p[P_MUTE] ? vis_lvl(pk[i]) : 0;   /* (as the TRACKS meters) */
        vs.lvl[i] = (int16_t)v;
    }
    vs.lvl[4] = (int16_t)vis_lvl(mpk);
    if (vis_style == VIS_TRAILS) {
        uint32_t t, v, cut;
        vis_notes_head = (uint8_t)((vis_notes_head + 1u) % 48u);
        memcpy(vis_notes[vis_notes_head], vis_note_hits, sizeof vis_notes[0]);
        memset(vis_note_hits, 0, sizeof vis_note_hits);
        fm1_irq_off();
        for (t = 0; t < NPART; t++) {
            if (trk_silent(&trk[t])) { memset(vis_notes[vis_notes_head][t], 0, sizeof vis_notes[0][0]); continue; }
            for (v = 0; v < NVOICE; v++) {
                const voice_t *n = &trk[t].v[v];
                if (n->active && n->gate && n->note < 128u)
                    vis_notes[vis_notes_head][t][n->note >> 5] |= 1u << (n->note & 31u);
            }
        }
        /* Rhythm follows the actual punch phase, not wet delay/reverb energy.
         * During the crossfade the dry notes are still audible. */
        cut = punch.cur == PX_GATE && punch.g == 32767 && punch.gp >= beat_samples() / 8u + 64u;
        fm1_irq_on();
        vis_drum_notes[vis_notes_head] = 0;
        for (t = 0; t < 16u; t++) if (pad_lit[t]) vis_drum_notes[vis_notes_head] |= (uint16_t)(1u << t);
        if (cut) { memset(vis_notes[vis_notes_head], 0, sizeof vis_notes[0]); vis_drum_notes[vis_notes_head] = 0; }
    }
    {                                               /* the wave's scale follows the peak */
        int32_t target = mpk < 1600 ? 1600 : mpk;
        vs.scale += (target - vs.scale) / 5;
    }
    switch (vis_style) {
    case VIS_SPECTRUM: case VIS_SPECTROGRAM: {
        int16_t b[32];
        vis_fft();
        vis_bands(b);
        if (vis_style != VIS_SPECTROGRAM) {
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
    case VIS_METERS:                                /* VU: fall 3 % a frame, peaks held a second */
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
UI_SMALL static void vis_scope(void)
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
UI_SMALL static void vis_spectrum(void)
{
    uint32_t k;
    for (k = 0; k < 32u; k++) {
        int32_t x0 = 4 + (int32_t)k * 7, h = vs.bar[k] * 210 / 1000, y, cy = 232 - vs.cap[k] * 210 / 1000;
        for (y = 232; y > 232 - h; y -= 4)          /* LED segments */
            cv_rect(x0, y - 2, 5, 3, TE_COL[k / 8u]);
        cv_rect(x0, cy - 1, 5, 2, C_WHITE);
    }
}
UI_SMALL static uint16_t vis_heat(uint32_t v)               /* 0..255: black, blue, green, yellow, white */
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
UI_SMALL static void vis_spectrogram(void)
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
UI_SMALL static void vis_meters(void)
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
/* the logo (tools/gen_logo.py at 0.86, centred on 120, 112), alive: the dial is the master, the sail's four
 * bands the tracks (growing with their level, flashing on a note), the boat rocks on the master's wave */


UI_SMALL static int vis_has(const track_t *t, uint32_t s)
{
    return is_drum(t) ? dstep_mask(&t->dstep[s]) != 0 : t->step[s].n && t->step[s].time != ST_REST;
}
UI_SMALL static void vis_trails(void)
{
    uint32_t i, t, n;
    for (n = 0; n < 128u; n += 12u)
        cv_rect(0, 216 - (int32_t)n * 3 / 2, 240, 1, C_LINE);
    for (i = 0; i < 48u; i++)
        for (t = 0; t < 3u; t++)
            for (n = 0; n < 128u; n++)
                if ((vis_notes[(vis_notes_head + 1u + i) % 48u][t][n >> 5] >> (n & 31u)) & 1u)
                    cv_rect((int32_t)i * 5 + (int32_t)t, 216 - (int32_t)n * 3 / 2, 5, 2, TE_COL[t]);
    cv_rect(239, 24, 1, 194, C_WHITE);
    cv_rect(0, 223, 240, 16, C_BLACK);
    for (i = 0; i < 48u; i++)
        for (n = 0; n < 16u; n++)
            if ((vis_drum_notes[(vis_notes_head + 1u + i) % 48u] >> n) & 1u)
                cv_rect((int32_t)i * 5, 223 + (int32_t)n, 5, 1, C_WHITE);
}
UI_SMALL static void vis_groove(void)
{
    uint32_t t, s;
    cv_text(8, 24, &FONT_S, "GRID / SWING / NUDGE", C_DIM);
    for (t = 0; t < 4u; t++) {
        track_t *p = &trk[t];
        uint32_t into,slen,len=trk_len(p),abs=trk_grid(p,&into,&slen),at=abs%len,base=at/8u*8u;
        int32_t y = 65 + (int32_t)t * 48;
        cv_rect(8, y + 15, 224, 1, TE_DIM[t]);
        /* Grid first: a shifted hit may land on the next cell's grid line. */
        for(s=0;s<8u;s++) cv_rect(14+(int32_t)s*28,y-8,1,30,C_LINE);
        for (s = 0; s < 8u; s++) {
            uint32_t idx = (base + s) % len, k, rats = 1, velocity = 100;
            int32_t x = 14 + (int32_t)s * 28, shift = p->micro[idx] * 28 / 64;
            uint32_t group=swing_group(trk_div(p)),step=abs-at+base+s;
            if(group && swings(trk_div(p))) {
                uint32_t within=step%group;
                shift+=clamp(p->p[P_SSWING]+song.g[G_SWING],0,100)*28*
                    ((step/group)&1u?group-within:within)/200;
            }
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
UI_SMALL static void vis_journey(void)
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
/* the visualiser, every other frame: the update, then both bands */
UI_SMALL static void vis_draw(void)
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
        case VIS_SCOPE: vis_scope(); break;
        case VIS_SPECTRUM: vis_spectrum(); break;
        case VIS_SPECTROGRAM: vis_spectrogram(); break;
        case VIS_METERS: vis_meters(); break;
        case VIS_TRAILS: vis_trails(); break;
        case VIS_GROOVE: vis_groove(); break;
        case VIS_JOURNEY: vis_journey(); break;
        default: vis_scope(); break;
        }
        if (!pass && (vis_name_t || ui.msg_t)) {    /* the style's name a second (or a message) */
            char n[8];
            const char *s = ui.msg_t ? ui.msg : VIS_NAME[vis_style % VIS_N];
            cv_rect(0, 0, 240, 20, C_BLACK);
            cv_text(120 - text_w(&FONT_S, s) / 2, 2, &FONT_S, s, C_WHITE);
            if (!ui.msg_t) {
                fmt_int(n, (int32_t)(vis_style % VIS_N) + 1);
                str_cpy(n + str_len(n), "/7", 3);
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
UI_SMALL static void vis_select(int32_t s)
{
    vis_style = (uint8_t)((vis_style + (s > 0 ? 1u : VIS_N - 1u)) % VIS_N);
    vis_name_t = 30;
    ui.force = 1;
    settings_later = 1;                                 /* (kept with the settings, once stopped) */
}
