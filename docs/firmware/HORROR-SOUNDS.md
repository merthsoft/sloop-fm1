# Horror-film synth voices

Four factory sounds use the existing synth engines, envelopes and LFOs:

| Sound | Engine | Browser kind | Try it with |
| --- | --- | --- | --- |
| NIGHT PAD | ANALOG | PAD | Low minor or diminished chords; detuned pulse layers and slow pitch/filter movement |
| SLASH STR | PHASE | INST | Short upper-register stabs or semitone clusters; alternating waveforms, detuning and fast vibrato |
| HEX CHOIR | VOICE | PAD | Held dissonant chords; breathy vowels changing slowly with lowered formant character |
| DREAD BELL | DIGITAL | FX | Sparse low notes or wide leaps; high-ratio metallic FM with a decaying envelope |

Open **SAVE → SOUNDS** and select the listed kind with knob 2, or browse the engine filters. Knob 1/PRESETS auditions; knob 3 right keeps the sound. Each also appears at the end of its engine's preset list. Use POLY for pad clusters if your track is currently mono; ordinary preset browsing preserves your existing voice-mode preference.

For an immediate horror texture, try C–E♭–G♭–A on NIGHT PAD or HEX CHOIR, and short C–D♭ stabs on SLASH STR. ENV controls the attack and release. LFO controls the pitch/filter movement; reduce its pitch destination for steadier tuning. Existing track effect preferences remain in effect when switching sounds.

## Implementation and checks

The bank adds four preset recipes and browser entries, with no new synthesis, sample data, audio buffers or DSP branches. ANALOG, DIGITAL and VOICE append in low RAM; PHASE appends to its existing main-RAM table. Factory count is 196, plus four optional SLICE presets. Every preceding engine-local index, browser identity, preset recipe and trim is retained.

Each new voice is checked across MIDI 48–84 and velocities 40/80/120 for audible bounded output, distinct recipes and clean release. Existing audio goldens remain exact, with four new reviewed entries. Raw and startup-restored factory records match; browser mapping and the frozen editor descriptor corpus pass.

Final half-dB trims are NIGHT PAD -2, SLASH STR -9, HEX CHOIR -20 and DREAD BELL 0. The choir's gain was reduced below the initial master-limiter plateau rather than leaving it disproportionately loud. Measurements and the isolated actual-firmware audition are retained privately in `build/horror-108/`. The audition order is NIGHT PAD, SLASH STR, HEX CHOIR, DREAD BELL, seven seconds each.

See [current headroom](HEADROOM-COMPARISON.md) for the combined target's actual memory totals. Existing engine cost still applies per active voice; reusing DSP does not make every preset equally cheap.

Final standard-phrase integrated levels are NIGHT PAD -15.49 LUFS, SLASH STR -16.09 LUFS, HEX CHOIR -16.28 LUFS and DREAD BELL -15.11 LUFS. Held sounds use the existing -16 LUFS target; melodic sounds use -15 LUFS. These are default full-mix measurements, not a guarantee for arbitrary chords, velocities or personal effect settings.
