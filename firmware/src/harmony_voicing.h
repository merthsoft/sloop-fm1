/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SLOOP_HARMONY_VOICING_H
#define SLOOP_HARMONY_VOICING_H
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
            for (j = 1; j < n; j++) {
                uint32_t k = j; uint8_t v = cand[j];
                while (k && cand[k-1] > v) { cand[k] = cand[k-1]; k--; }
                cand[k] = v;
            }
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
