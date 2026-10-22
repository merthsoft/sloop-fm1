/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SLOOP_HARMONY_VOICING_H
#define SLOOP_HARMONY_VOICING_H
static void harmony_sort_notes(uint8_t *notes, uint32_t n)
{
    for (uint32_t j = 1; j < n; j++) {
        uint32_t k = j; uint8_t v = notes[j];
        while (k && notes[k-1] > v) { notes[k] = notes[k-1]; k--; }
        notes[k] = v;
    }
}

/* Sorted, distinct MIDI input, at most four voices. Inversions rotate an outer
 * voice beyond the opposite edge, including octave/sparse chords. A rotation
 * that would leave MIDI range is refused; no note is clamped or discarded.
 * OPEN raises alternate voices, WIDE moves the outer voices apart. Apply after
 * automatic voice leading so these explicit choices remain audible. */
static void harmony_shape(uint8_t *notes, uint32_t n, int32_t inversion, uint32_t spread)
{
    if (!n || n > 4u) return;
    if (inversion > 3) inversion = 3;
    if (inversion < -3) inversion = -3;
    if (spread > 2u) spread = 2u;
    while (inversion) {
        uint32_t i = inversion > 0 ? 0 : n-1u;
        int32_t v = notes[i];
        do { v += inversion > 0 ? 12 : -12; }
        while (inversion > 0 ? v <= notes[n-1u] : v >= notes[0]);
        if (v < 0 || v > 127) break;
        notes[i] = (uint8_t)v;
        harmony_sort_notes(notes, n);
        inversion += inversion > 0 ? -1 : 1;
    }
    if (n < 2u) return;
    if (spread == 1u) {
        /* Preserve the input indices until all alternating voices are moved. */
        for (uint32_t i = 1; i < n; i += 2) {
            uint32_t v = notes[i], j;
            do {
                v += 12;
                for (j = 0; j < n && notes[j] != v; j++) ;
            } while (j < n);
            if (v < 128u) notes[i] = (uint8_t)v;
        }
    } else if (spread == 2u) {
        if (notes[0] >= 12u) notes[0] -= 12u;
        if (notes[n-1u] < 116u) notes[n-1u] += 12u;
    }
    if (spread) harmony_sort_notes(notes, n);
}

/* Same nearest-inversion policy for live chords and stateless ROM progressions. */
static void harmony_voice_lead(uint8_t *notes, uint32_t n, const uint8_t *prev,
                               uint32_t pn, int octave_pair)
{
    uint8_t best[4], cand[4];
    int32_t bcost = 0x7FFFFFFF, inv, oct;
    uint32_t j;
    if (!n || !pn || n > 4u || pn > 4u) return;
    for (inv = 0; inv < (octave_pair ? 1 : (int32_t)n); inv++)
        for (oct = -1; oct <= 1; oct++) {
            int32_t cost = 0; uint32_t valid = 1;
            for (j = 0; j < n; j++) {
                int32_t v = notes[j] + 12 * oct + (j < (uint32_t)inv ? 12 : 0);
                if (v < 24 || v > 108) { valid = 0; break; }
                cand[j] = (uint8_t)v;
            }
            if (!valid) continue;
            harmony_sort_notes(cand, n);
            /* Sparse scales can span an octave within one chord. Rotating such
             * a chord must not collapse two voices onto the same MIDI note. */
            for (j = 1; j < n; j++) if (cand[j] == cand[j-1]) valid = 0;
            if (!valid) continue;
            for (j = 0; j < n; j++) {
                int32_t d = (int32_t)cand[j] - prev[j < pn ? j : pn - 1u];
                cost += d < 0 ? -d : d;
            }
            if (cost < bcost) { bcost = cost; for(j=0;j<n;j++) best[j]=cand[j]; }
        }
    if (bcost != 0x7FFFFFFF) for(j=0;j<n;j++) notes[j]=best[j];
}
#endif
