# More console and worn-digital sounds

Eight factory presets append to existing engine banks and the sound browser. All earlier engine-local preset IDs, factory browser positions and factory favourite identities remain unchanged. The normal factory count increases from 171 to 179; the optional SLICE build adds its existing four presets. USER sounds still follow factory sounds in the combined list, so their absolute combined list positions move by eight; stable user-slot addressing remains available.

| Preset | Engine / browser kind | Character |
| --- | --- | --- |
| NES REED | LOFI / CHIP | Softer 25% stepped pulse lead with restrained vibrato. |
| NES BUBBLE | LOFI / CHIP | 75% stepped pulse, looping frame-volume decay and rapid octave movement. |
| GB MALLET | LOFI / CHIP | Short, rounded four-bit harmonic wavetable strike. |
| GB VOX | LOFI / CHIP | Sustained four-bit vowel wavetable with a slower onset and gentle vibrato. |
| MD BASS | DIGITAL / CHIP | Mega Drive-inspired four-operator FM bass, with a bright attack, feedback and a lower register. |
| MD BRASS | DIGITAL / CHIP | Sustained FM brass using parallel operator stacks and a little pitch movement. |
| WORN EP | LOFI / LOFI | Rounded, lightly downsampled electric-keyboard-like decay. |
| DUST ORGAN | LOFI / LOFI | Grainy wavetable organ with vibrato and restrained chorus/reverb. |

Use SAVE > SOUNDS, then KNOB 2 KIND = CHIP or LOFI. KNOB 1/PRESETS auditions the filtered list; KNOB 3 right keeps the sound, and KNOB 4 right adds a favourite. The existing engine-local preset controls also reach the appended sounds. Every recipe remains editable using existing CHIP/MOTN or OPS/MOD, voice, envelope, LFO and FX pages. These are console-inspired interpretations using the existing engines, not new console emulators.

No samples, wave tables, oscillators, effect processors, per-sample branches or audio buffers are added. Preset records restore into the existing low-RAM factory bank. The eight records add **288 B low RAM**, and packed preset names/data add **64 B main static RAM** in this build. Existing preset trims are preserved by name; only the eight new records receive trims from the established phrase/K-weighted loudness workflow. This is a calibration aid, not a claim that short mallet strikes and held chords have identical perceived loudness. The legacy workflow folds stereo and uses its existing weighting coefficients; controlled stereo loudness work remains separate.

## Space integration

The companion audits measured cold sharing at 80 B, the conservative PHYS/mixer batch at 320 B, and startup-literal layout at 108 B, each independently against the frozen dirty .99 baseline. These figures are not an additive combined claim. This build integrates:

- Word-based drum-setting project packing, with identical bytes/checksum, and one byte-FNV helper shared by project and step hashing.
- Shared PHYS MODAL/MEMB setup finalization/dispatch, accumulator clearing and DAC/USB limiter/output conversion. Original sample loops, limiter states, USB reset ordering and failed-worker clearing remain intact. No new per-sample helper is introduced.
- Lexical assignment of existing literal roots to the same bounded main/pool destinations, then reverse-lexical group layout. Restored bytes and runtime decoder remain unchanged; only startup representation/address layout changes.

The optional extra TRIO/NOISE/drum filter sharing and FORMANT helper remain deferred because their small additional savings have loop/register/stack tradeoffs. No more Android functionality or other features are removed.

The final normal **2.8.0.100** image is **583,396 / 584,336 B (940 B free)**, **332 B smaller than .99 even with the eight sounds**, new trims, DUCK compatibility fix and version packing. Main static RAM is **97,676 / 98,304 B (628 B free)**, low RAM **29,216 / 29,392 B (176 B free)** and pool **335,424 / 344,064 B**, retaining its 8,192 B reserve. Low RAM is the tighter constraint for more restored preset records.

## Compatibility correction

The cold audit found that .98/.99 always wrote the optional DUCK tag, even with default KICK/eighth/SNAP settings. That changed an upstream project's DUCK high byte and checksum on resave despite unchanged musical values. Capture now leaves the legacy depth word untouched at defaults and tags only nondefault trigger/recovery/shape. Original upstream fixture equality is checked without modifying or normalizing its oracle; configurable ducking still persists.

## Evidence

`build/chip-sounds-100/` contains renders, calibration report, target logs and final checks. `tests/chip_presets_test.c` checks names, valid recipe ranges, differences from older recipes, nonzero/bounded output and writes an eight-sound demo WAV. The browser name/position oracle appends expected rows, preserving the full original prefix. Raw/restored factory preset comparison checks every name and complete field record. Existing audio goldens and persistence oracles remain unchanged.

All seventeen final host programs pass, including the focused preset render, complete browser/UI checks, 200 unchanged audio goldens, fourteen-scene/all-mask dual-core parity with chaotic execution and forced timeout, PHYS/limiter full-state sanitizer comparisons, 200,000 arbitrary-byte drum-setting/hash cases, 42,000 legacy import/output/rejection cases, project/NOR/snapshot persistence and unmodified upstream byte-identical resave fixtures. All 179 normal-build preset records/names are identical between raw C and boot restoration. All 171 original factory records and 175 existing trim entries (including optional SLICE) remain byte-identical/by-name unchanged respectively.

The private host driver initially omitted the BOOT_TABLES dependency while compiling the boot-restoration test. It stopped before running the suite; after correcting those flags, the restored build and every program passed. Initial compile output is retained alongside the complete resumed log. No oracle or golden value was relaxed.

The eight newly rendered preset hashes were then added deliberately: all **200 existing hash entries are unchanged**, and a normal rerun verifies **208/208 recorded goldens**, with no missing/new/changed renders or health/routing/CPU failures. Final target loop/inlining/LOFI guards, 1,411 complete boot-literal object/address restores with guards and UBSan, native/WASM parity and browser playback/editing, name-alphabet/packing tests and all 16 Pages deployment tests pass. The local installer is verified serving .100. No physical hardware sound/stack/deadline qualification is claimed for this new batch.
