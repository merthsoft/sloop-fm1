# Reverse sounds and measured headroom

## Scope

REV is the fifth shared delay type, after DIGI/TAPE/CRUSH/DUB. Two new PHASE presets, REV LEAD and REV GLASS, append to the engine and browser lists without moving older IDs. They use rising amplitude envelopes with zero sustain and short release; they are reverse-style synthesized sounds, not reversed sample playback. No reverse reverb or additional sample-engine direction control is included.

## Delay implementation

The existing 65,536-sample int16 ring is reused. Two backwards heads begin half a window apart. Each starts immediately behind the writer and walks backwards; its age grows by two samples per rendered sample. Complementary triangular weights hide the splices. Window length is capped at 32,768 samples (~0.743 s at 44.1 kHz), keeping every read behind the writer and inside retained history. TIME controls fragment length rather than promising the ordinary forward echo’s onset spacing.

A 32-bit phase accumulator advances from one block-level integer division. There is no new per-sample division, audio allocation or sample decoder. Mode changes crossfade with the forward tap over 128 samples. Existing 64-sample time-change crossfades apply to both read modes. The delay’s existing tone/feedback loop and bounded int16 writes remain. The two overlapping heads can produce audible granular texture; this is not phrase capture or studio reverse reverb. Audio control remains on core 0 after worker joins.

Delay type uses the three low bits of the existing 0xED00 project tag. The per-track duck bypass mask remains in bits 3–5. Old delay IDs and default untagged upstream saves retain their meaning.

## Space saving and capacity

Before adding reverse, the application-only `-mllvm -inline-threshold=0` candidate was remeasured against .101: 583,896 → 583,720 B, **176 B saved**, with unchanged static RAM. Timer reads and flash IRQ save/restore are explicitly always-inline; the loader retains its original flags. This is the previously audited compiler-specific choice, remeasured on current sources rather than its old independent 320 B result. It can add calls in event/mixer setup; passing static guards is not physical deadline qualification.

Final .102, including reverse delay, both presets and their calibrated trims: **583,880 / 584,336 B**, **456 B free**. Main static RAM 97,724 / 98,304 B; low RAM 29,288 / 29,392 B; pool 335,424 / 344,064 B with the 8,192 B reserve retained. These are actual combined target numbers. No feature removal or runtime data compression funds this batch.

## Verification

- Reverse-window reference checks cover signed full-scale data, ring wrap and window bounds, forward identity, actual bus rendering, mode/time changes and maximum feedback under UBSan (existing fixed-point shift exception).
- All 208 prior audio hashes remain identical. Only the two new preset renders are added, followed by a normal verification run.
- All 183 existing preset trims remain unchanged; only the two new sounds are calibrated with the existing loudness tool.
- Routing/project tests exercise every delay type with every duck mask through full-project and section adoption. Original upstream fixture byte round trips remain unchanged.
- Target placement, DSP budget/inlining and LOFI sample-loop guards remain at their existing limits. Boot/literal restoration and native/WASM parity are checked on the final build.

Evidence is retained in ignored `build/reverse-102/`. Device listening and worst-case deadline testing remain necessary for the new reverse mode and compiler policy; no hardware was flashed automatically.
## More swelling voices

Three additional factory presets complement REV LEAD and REV GLASS:

| Preset | Engine | Character | Approximate rise |
| --- | --- | --- | --- |
| REV STR | ANALOG | Detuned strings with an opening filter | 0.59 s |
| REV HAZE | LOFI | Dusty wave-RAM pad | 1.06 s |
| REV REED | PHASE | Nasal reed-like lead | 0.19 s |

Use SAVE → SOUNDS to browse them: REV STR is under PAD, REV HAZE under LOFI, and REV REED under LEAD. They also appear at the end of their engine preset lists. Existing browser and engine-local identities remain unchanged.

Hold a note through the rise to hear its full crest. Each envelope swells from silence, then closes while the key is still down; release is short. Short sequencer gates deliberately cut the swell early, so lengthen a note or use ties when you want the full rise. ENV → ATK adjusts the swell duration; ENV SYNC can put it on the beat. These use existing oscillators and envelopes, rather than reversing recorded audio. No additional audio buffers or DSP loops are introduced.

All three get individual loudness trims. Range/velocity tests verify rising energy, silence after the crest while held, and clean note-off; browser tests retain the original identity prefix. PHASE's restored preset metadata moves from low RAM to main RAM to accommodate the appended recipes without exhausting low RAM.

The combined swelling-preset and violin-refinement build uses **582,260 / 584,336 B**: **2,076 B flash free**, only **64 B** above the preceding headroom build. Main static RAM is **97,852 / 98,304 B**, low RAM **28,928 / 29,392 B**, and pool **335,424 / 344,064 B**. The main-RAM increase includes the relocated PHASE metadata and new names; the pool reserve is unchanged. All 213 audio goldens pass after reviewing only the three new entries and the intentional VIOLIN change.
