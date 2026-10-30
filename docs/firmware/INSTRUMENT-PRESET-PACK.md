# Instrument and console-inspired preset pack

## Follow-on status

The later LOFI experiments append DUST KEYS, WORN PAD, TOY BELL and DUST BASS
without changing the fourteen voices below. Eight more console/worn-digital presets now bring the current normal factory count to 179; see [the expansion](CONSOLE-SOUNDS-EXPANSION.md). The earlier four-LOFI follow-on brought it to 171;
all eighteen appended voices pass range/velocity/release tests. See
[LOFI implementation and current measurements](LOFI-MASTER-DESIGN.md).
The measurements below describe the original fourteen-voice batch.

## Scope

Start with fourteen additional factory presets, using existing engines only.
There are no new samples, wave tables, synthesis algorithms, audio buffers or
instrument-specific macro pages in this batch. Existing factory sounds are kept.

| Collection | Presets | Engine |
| --- | --- | --- |
| NES-inspired | NES PLS12, NES PLS50, NES PLUCK, NES TRI | LOFI stepped pulse/triangle/envelope |
| Game Boy-inspired | GB WAVE BS, GB HOLLOW, GB ORGAN, GB ARP | LOFI existing 32-sample four-bit wave RAM |
| Synth winds | CLARINET, BASSOON | DIGITAL four-operator FM |
| Synth strings/reeds | VIOLIN, OBOE | PHASE distortion |
| Plucked instruments | HARPSICHORD, OUD | PHYS string model |

These are playable interpretations, not cycle-accurate console emulation or
sampled orchestral instruments. Existing tone/envelope/voice/effect pages remain
the editing interface. Save a variation in USER; factory presets remain intact.

## Identity and compatibility

Append to each engine's preset table and to the end of the canonical browser
bank, after any optional SLICE entries. Preserve all 153 existing production
factory browser indices, engine-local indices and favourite identities. Added
factory sounds precede user sounds in the combined list, so absolute combined
Program Change indices for user sounds move; bank 8 retains stable user-slot
addressing. Do not change project layout, parameter IDs or synthesis output for
existing presets. The connected web editor reads names and parameters from the device. The offline mock catalog remains a partial demonstration bank; extending it into a complete factory browser is separate work. Matching WASM DSP is rebuilt with the firmware.

## Budget and validation

The preceding target is 582,048 / 584,336 B, with 2,288 B flash free and 1,216 B
low RAM free. Target preset records occupy 36 B each after boot restoration, so
fourteen records require 504 B low RAM before alignment; flash must be measured
with the actual compressed boot tables. Do not infer linked flash from source
line counts or remove features without first measuring this data-only pack.

Measure new presets using the existing loudness phrase and K-weighted pipeline,
leaving existing trim values untouched. Check tuning/playable range, useful
envelopes, finite/bounded output, release cleanup, distinct chip timbres and
four-track playback. Freeze old browser identities and old audio hashes; add
new golden renders deliberately. Check boot-packed/raw preset equivalence,
browser/favourites/MIDI, native/WASM parity and final target placement/capacity.

The broader collection browser, friendly macro controls, exact console emulation
and sample-based SNES/orchestral collections remain separate designs.

## Implemented result

The target is **582,424 / 584,336 B**, leaving **1,912 B flash**. Relative to
its preceding build this costs **376 B flash** and **504 B low RAM**. Low RAM
is 28,784 / 29,496 B; main static RAM remains 86,144 / 98,304 B and the audio
pool remains 332,876 / 344,064 B. No features were removed.

Validation covers 167 complete factory preset records through raw and boot-packed
restoration, the unchanged original 153 browser identities, pitch/velocity/release
checks for all fourteen additions, distinct render fingerprints and a mixed
three-synth/drum stress phrase. The 196-render regression suite passes; all 182
previous hashes and original trim values are unchanged. Matching preview DSP
passes the existing 32 native/WASM profiles, and target DSP cost/inlining checks
pass. Device listening remains the final judgement of instrument character.

Only the new sounds were level-trimmed. The existing phrase measurement puts
these approximately between -15.5 and -14.2 dB in its K-weighted loudness metric.
That is a comparison against this tool's established phrase, not a claim that
all notes, articulations and chords have identical perceived volume. The legacy
measurement filter coefficients were retained to avoid recalibrating old sounds.

The offline mock editor still has its smaller demonstration catalog; it does
not expose the full appended bank. A connected editor receives the real bank.

## Clarinet onset refinement

CLARINET keeps its odd-harmonic FM topology but uses less modulation at onset,
a quicker modulation decay and a softer amplitude attack (IDX 18, MDEC 64,
ATTACK 24). This removes the metallic attack burst without changing the DIGITAL
engine or any other preset. The existing level trim is retained: rendered
sustain level stays within about 0.4 dB of the previous patch across C4–C6.
Only the clarinet audio golden changes. The revised patch passes the instrument
range/velocity/release checks; the matching preview build passes the existing
33 native/browser engine and effects profiles.
