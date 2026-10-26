# MerthSloop firmware 2.8.0.77 — saved voice budget

- HOME → AUDIO → VOICE BUDGET now survives power cycles through the existing verified device-settings transaction. Close HOME settings to save; changes made during playback are deferred until transport stops. Older records default to 8 / 12; invalid levels clamp safely. Upstream song formats are unchanged.
- Target: 584,328 B, 8 B flash free; main static 90,336 B. Fault-injected settings commits, restart of every level and backup retention pass.

# MerthSloop firmware 2.8.0.76 — About restored and evolution octave bounds

- Standard About credits/layout/colors and version/author/core information restored. The 95% guard and automatic eligible-part core balancing remain active independently of the optional diagnostic screen. Runtime voice-budget choices remain; power-on still defaults to 8 / 12.
- LFO → EVOLVE 2: knob 1 LOW OCT / knob 2 HIGH OCT (-1..9), default 3 / 5 (C3–B5). Endpoints follow each other when crossed. Bounds limit future evolution attempts without cutting or retuning held notes; manual out-of-range notes remain as played. Per-track runtime controls, no saved-format change.
- A shared About-row drawing path/runtime field resolver and explicit keyboard/evolution handler boundaries fit both requests without musical cuts or new startup/sample decoding.
- Normal target 584,296 B (40 B flash free), main static 90,336 B (7,968 B free), low RAM 28,928 B and pool 331,852 B. The earlier audible drone fix remains; residual hardware deadline overruns are not declared resolved.

# MerthSloop firmware 2.8.0.75 — diagnostic core-balancing trial

- Longer .74 hardware run accumulated eight new DMA misses, two CPU sheds and a 6,039-us peak. The threshold change alone was insufficient for deadline qualification.
- Diagnostic builds only: core 1 receives the estimated heaviest eligible sounding synth part plus drums, using the preceding block's existing render count. Global-RNG/PHYS/multiple-FORMANT exclusions, manual masks and fallback remain intact. The 95% shedding trial remains; normal builds retain their previous split and 85% guard.
- ABOUT has compact STEALS / SHEDS / RESTORE / MAX US / LATE / C1MASK snapshots. OCT+ refreshes; OCT- returns. Capture while playback continues, on the same boot.
- Diagnostic target 584,316 B (20 B flash free), main static 90,368 B (7,936 B free); normal target 584,300 B (36 B flash free), main 90,336 B. Low RAM/pool unchanged. No musical feature cuts or saved-format change.
- Eight-scene, thirteen-split audio parity includes the five-drone workload, asynchronous workers and forced timeouts. Goldens, ownership, UI and focused sanitizer checks pass; hardware timing remains pending.
- [Trial behavior and capture instructions](../../docs/firmware/VOICE-BUDGET-EXPERIMENT.md#on-device-dropout-diagnostics).

# MerthSloop firmware 2.8.0.74 — diagnostic overload-threshold trial

- Hardware .73 captures confirmed 20 CPU sheds and 10 drone restores at a 5,373-us peak, with LATE HALVES unchanged at 7 and CPU1 running without timeouts. Shared admission was already set to 48 units.
- Diagnostic builds only (`--audio-diag`) trial shedding after two consecutive halves above 95% of the wall-time deadline, rather than 85%. Normal builds retain 85%. Victim selection, recovery and admission limits are unchanged.
- Real-ISR tests reproduce shedding at the reported time with the normal guard and retain all three notes with the trial. Exact thresholds, isolated/consecutive spikes, nested work and sustained overruns pass; trial passes UBSan. Hardware deadline qualification remains pending.
- Diagnostic target 584,264 B (72 B free), static RAM unchanged. [Hardware evidence and trial instructions](../../docs/firmware/VOICE-BUDGET-EXPERIMENT.md#hardware-capture-and-threshold-trial).

# MerthSloop firmware 2.8.0.73 — local dropout diagnostic build

- Optional `--audio-diag` build replaces ABOUT credits with snapshots of steals, CPU sheds, drone restores, wall-time peak, DMA misses, CPU1 state/timeouts, sounding gates and logical held pitches. OCT+ refreshes; OCT- returns. Counters last for the current boot. Normal build defaults this hook OFF.
- No audio-policy change. This investigates three latched LOFI drone notes dropping even at the maximum shared budget. The cause remains unconfirmed until hardware counters are collected.
- Diagnostic target 584,264 / 584,336 B (72 B free); static RAM totals unchanged. The current local installer contains this diagnostic build; browser preview retains normal ABOUT.
- [Capture instructions and field definitions](../../docs/firmware/VOICE-BUDGET-EXPERIMENT.md#on-device-dropout-diagnostics).

# MerthSloop firmware 2.8.0.72 — wider shared voice budget

- HOME → AUDIO → VOICE BUDGET (knob 4) now offers 8 / 12, 10 / 15, 12 / 18, 14 / 21, and 16 / 24: 24–48 weighted units. Knob stops at the ends; OCT+ cycles. Power-on default stays 8 / 12.
- Uses existing voice slots; no added RAM or saved-format change. Engine-specific caps and CPU overload shedding remain. GRAIN stays three notes per track and VOICE four.
- Every heavy/cheap admission boundary and menu endpoint/wrap is tested, including 24 simultaneous LOFI voices. Single/two-core recovery checks pass; hardware deadline qualification remains pending.
- Target 584,300 / 584,336 B: 36 B flash free; main static 90,336 B. The selector extension adds 20 B flash and no RAM versus .71.

# MerthSloop firmware 2.8.0.71 — adjustable voice budget and drone recovery

- HOME → AUDIO → VOICE BUDGET, knob 4: default 8 / 12 (24 units) or experimental 10 / 15 (30 units). Runtime-only, resets at power-off. Engine caps, physical slots and CPU shedding remain.
- Reproduce GRAIN + VOICE + LOFI drone pressure with Vinyl off/on. Higher budget keeps the 27-unit scene intact. Missing drone pitches survive both cross-track fades and own-slot reuse; recovery never steals or sends extra MIDI/recording events. Fresh attacks supersede old pending pitches, and natural one-shots are not repeated.
- Share overload/admission victim categories, preserving quiet released-note priority and bass/lead protection.
- Integrate the prior ordered native-clamp audit through the HAL, with portable fallback and original dynamic/inverted-bound contract. Signed boundary/register tests accompany target DSP checks.
- Final target 584,280 / 584,336 B: 56 B free; main static 90,336 / 98,304 B (7,968 B free), +400 B versus .70. Low RAM/pool unchanged. No saved-format change or added voice slots.
- Hardware listening, peak/deadline and stack qualification remain pending; this is an optional device-evaluation setting.

# MerthSloop firmware 2.8.0.70 — sustain characters

- ENV → SUSTAIN adds PIANO (faster treble loss), STRINGS (gentler darkening) and LOFI (released-tail sample hold/reduced resolution). PEDAL stays unchanged at default DAMP 0 / AMOUNT 100%. Held/new attacks remain clean.
- Knob 3 DAMP and knob 4 AMOUNT; zero amount gives ordinary release. All settings remain per-track/runtime-only and reset at power-off.
- 128-byte per-render stack scratch and private voice tone state; no feedback/history buffer or saved-format change.
- Final image 584,204 / 584,336 B: 132 B flash free. Main static 89,936 / 98,304 B (8,368 B free), +320 B versus .69. Low RAM/pool unchanged.
- Render comparisons cover distinct tails, identical held attacks, amount/damping response, decay/HOLD, engine matrix and UBSan. Final ownership, multi-core, UI, golden and browser checks accompany the build. Hardware listening/deadline/stack qualification remains pending.

# MerthSloop firmware 2.8.0.69 — voice admission and cold-code sharing

- Shared weighted admission: up to twelve LOFI/WHEEL voices, eight normal voices, intermediate mixed populations; retain eight slots per track and existing engine caps, except FM6 now eight. Drum voices remain separate.
- Fix a steal-fade same-note retrigger bypass: fading slots must reacquire admission before restarting.
- Share project validation/initialization, settings commit, A/B storage selection and editor notifications without changing saved formats or factory data. Startup-string packing remains excluded.
- Final target 583,776 / 584,336 B: 560 B flash free, 288 B recovered versus .68. All static RAM totals unchanged.
- Assertions-enabled capacity, sustain, storage fault injection, sanitizers and browser/native parity cover integration. Expanded-polyphony device deadlines remain unqualified.

# MerthSloop firmware 2.8.0.68 — sustain foundation and synced evolution

- ENV → SUSTAIN: TYPE OFF/PEDAL, DECAY 0.5S–32S or HOLD. Engine-held tails
  retain natural envelopes/loops while logical gates remain released. New notes
  and voice stealing retain priority; no added voices or audio buffers.
- OFF/STOP/panic/retrigger cleanup, MIDI note-off timing and independent ties
  are covered. PIANO/STRINGS/LOFI, damping/amount and persistence remain later work.
- EVOLVE adds 1S/2S and eleven synced divisions from sixteenth-note triplets to
  16 bars. Its live cycle bar follows the scheduler; synced cycles pause with
  transport and consume each boundary once, including external-clock phase.
- Integrate the isolated minimum-cost build-time LZ4 parser audit; retain the
  original decoder, factory bytes and fallback for inputs above 8 KiB.
- Final target **584,064 / 584,336 B**, **272 B flash free**. Main static
  **89,616 / 98,304 B**, 8,688 B free; low RAM/pool unchanged. Combined delta
  versus .67: +1,100 B flash, +160 B main RAM.
- Decay durations, engine/source ownership, MIDI packets, ties, STOP, allocator,
  synced boundaries and native UI pass; sanitizer/two-core, unchanged goldens,
  full UI/fuzz, restored-data, target and browser checks cover the integration.
  Hardware listening/deadline/stack testing remains pending. Voice-budget audit
  candidates remain uninstalled.

# MerthSloop firmware 2.8.0.67 — slow drone-note evolution

- LFO → EVOLVE: MODE / CYCLE / CHANCE / RANGE. With DRONE/BOTH, replace at
  most one held pitch per 5–160-second cycle, using ROOT/SCALE and 1–7 scale
  steps per change. OFF keeps the reached harmony; STOP clears held notes.
- Shared drone toggle path preserves MIDI channel/ownership and other voices;
  skip duplicates, pitch limits and shared physical/MIDI/sequencer pitches.
  Normal REC stores sounded notes; otherwise no pattern/patch writes. Runtime-only.
- Target **582,964 / 584,336 B**, **1,372 B flash free**. Adds 752 B flash and
  48 B main static RAM; low RAM/pool unchanged, no new audio buffer.
- All 200 audio goldens unchanged. Scale/root/bounds, timing/chance/OFF,
  retained-voice bytes, MIDI balance, recording, independent tracks/RNG, real UI,
  UBSan, existing drone/progression, redraw and 20,000-frame fuzz checks pass.
  Hardware listening/deadline/stack measurements remain pending.

# MerthSloop firmware 2.8.0.66 — startup compression and open HOME lanes

- Search up to 64 earlier matches and use lazy matching when building startup
  LZ4 data; retain the fast compressor's output whenever it is smaller.
- Same firmware block decoder and restored bytes; no new playback processing,
  buffers, RAM allocation or file/protocol changes. Loader compression is unchanged.
- Remove the opaque gray HOME lane panels so the grid and traveling highlight
  remain visible between notes. Track-number badges retain black backing.
- Target **582,212 / 584,336 B**, **2,124 B flash free**: **768 B recovered**
  versus .65. Main static RAM remains 89,408 B; low RAM/pool are unchanged.
- All 200 audio goldens unchanged; restored tables/zones and 171 presets match
  raw originals. Compressor format/window/tail/fallback tests, UI redraw/foreground
  isolation and target hot-loop checks pass. Hardware validation remains pending.

# MerthSloop firmware 2.8.0.65 — HOME accents and empty-step moves

- HOME adds eight fixed stars, a pulsing horizon glow and a beat-driven wireframe
  highlight using the existing redraw clock; no new animation state or buffer.
- OCT + knob 3 can move into silent steps containing stale note data or timing
  left by recording/deletion. Actual note/tie collision rules and parameter-lock
  protection remain; undo/redo restores the original steps.
- UI refresh/card isolation and 20,000-frame navigation fuzz pass; move regression
  covers stale rests, timing/condition leftovers, lock rejection and undo/redo.
- Target 582,980 / 584,336 B: **1,356 B flash free**, static RAM unchanged.
  Hardware validation remains pending.

# MerthSloop firmware 2.8.0.64 — extended startup packing

- Removed the independent REV HOLD page and processing by request. Upstream's
  existing FREEZE punch-in is retained.
- Restore sample-zone metadata and compact preset names at cold startup using
  the existing decoder; direct access during playback, unchanged sample bytes.
- Share the two bounded chord scale walks; remove duplicate TRACKS dispatch.
- Current cold-state conversion was tested and rejected: +64 B flash.
- Target **582,804 / 584,336 B**, **1,532 B flash free**: 1,196 B recovered
  against .63. Main static RAM **89,408 / 98,304 B** (8,896 B free); low and
  audio pool remain unchanged. No file/protocol or factory-ID changes.
- Raw/boot byte oracles, name fallback and 200 audio goldens in each data
  configuration pass. UI/fuzz, ownership, split-core, native/WASM and target
  checks cover the retained behavior. Hardware cold-boot checks remain pending.

# MerthSloop firmware 2.8.0.63 — independent reverb tail HOLD

- Native FX → REV HOLD: FREEZE / TYPE / SIZE / DAMP. Captures the existing
  reverb tail and blocks new input while preserving dry playing.
- Uses existing reverb buffers; exact FDN line replay avoids frozen-tail
  decay from interpolation/rounding. SPRING uses its existing held path.
- FREEZE OFF/STOP/full project panic releases it. Track/sound browsing keeps
  it; TYPE changes wait until release. Runtime-only, no saved-format change.
- All six ten-second holds stay within 2% energy drift; new-input isolation,
  unchanged FDN bytes/dry samples, UI routing and lifecycle checks pass.
- Upstream punch-in FREEZE remains separate; existing audio/UI/reverb/split-core
  and native/WASM checks pass. Hardware listening is pending.
- Target 584,000 / 584,336 B: **336 B flash free**, RAM unchanged from .62.

# MerthSloop firmware 2.8.0.62 — tap-toggle DRONE

- LFO → DRIFT knob 1: OFF / DRIFT / DRONE / BOTH. Tap pitches to add/remove
  them without reattacking the others. Existing held-note storage is reused.
- Pitches and outgoing MIDI channels survive octave/track changes; selected-note
  lights follow the mapped octave. STOP and source/preset/route changes clean up.
- Selecting DRONE takes over live ARP/chord input without stopping transport or
  overwriting sequences. MIDI and sequencer gates cannot cut a held drone pitch.
- Use POLY and an ENV sustain level for a sustained chord bed; ordinary voice
  budgets remain. Mode/notes are runtime-only, no project/protocol format change.
- Target 583,716 / 584,336 B: **620 B flash free**, static RAM unchanged.
- Drone source/ownership tests, unchanged audio goldens, native/WASM, legacy
  latch/scale/progression, UI/fuzz and split-core checks pass; hardware pending.

# MerthSloop firmware 2.8.0.61 — audible and visible DRIFT

- Doubled full-scale depth to approximately ±64 control units; fresh defaults
  are OFF, 10 seconds and about 50% amount. Smooth fades remain.
- Live audio-owned movement meter and cycle progress replace duplicated values.
- LOFI responds to negative and positive DRIFT around TONE while retaining
  existing envelope/LFO behavior and a stable minimum filter coefficient.
- 200 unchanged default-off goldens; signed LOFI/manual-tone equivalence,
  motion bounds, live redraw, UI fuzz, split-core and native/WASM checks pass.
- Target 583,180 / 584,336 B: **1,156 B free**; static RAM unchanged from .60.
  Hardware listening remains pending.

# MerthSloop firmware 2.8.0.60 — first ambient slice

- New native LFO → DRIFT page: independent smooth random TONE/SHAPE/PAN motion,
  5–160-second cycles, bounded depth and fading amount/OFF/target transitions.
  Default OFF; runtime-only, no project-format or Android protocol change.
- Appended sustained STILL WATER, WIDE SKY and DUSK starters (27 total).
- DRONE tap-toggle mode remains planned, pending physical/MIDI note ownership.
- Application 582,660 / 584,336 B: **1,676 B free**, 664 B above .59.
  Main static RAM 86,432 B; low RAM 28,928 B; pool 331,852 B.
- 200 unchanged audio goldens, actual DRIFT audio/UI tests, starter fingerprints,
  UI/fuzz, split-core, native/WASM parity and target budget checks pass.
  Hardware listening and timing remain pending.

# MerthSloop firmware 2.8.0.59 — headroom integration

- Shared exclusive insert state, sample-upload decoding scratch and FFT scratch.
- Portable C clamp specialization preserves dynamic/inverted bounds.
- Exact grain window and FM6 envelope tables restore once at boot; preset field
  packing improves without changing any factory values or user save formats.
- Application 581,996 / 584,336 B: **2,340 B free**, **1,392 B recovered**.
  Main RAM 86,320 B; low RAM 28,928 B; pool 331,852 B (1,024 B recovered).
- Raw/boot audio goldens, boot bytes/presets, state lifetime, UI/fuzz and split-core
  checks pass. Hardware timing/stack validation remains pending.
- Assembly and shared analog-renderer alternatives are not included.

# MerthSloop firmware 2.8.0.58 — sampler drive range

- Corrected above-unity fixed-point gain overflow in SMPLR DRIVE. Squared the control curve so low settings stay gentle and high settings retain heavy clipping.
- Loud-bus regression checks 20%, 50% and full drive; 20% stays below 3 internal units RMS error on unclipped input.
- Includes the softer CLARINET onset from the preceding build.
- Target 583,388 / 584,336 B: 948 B flash free; static RAM unchanged.

# MerthSloop firmware 2.8.0.57 — clarinet articulation

- Softened CLARINET's metallic onset with lower FM index, quicker modulation decay and a gentler attack. Other presets and engine DSP are unchanged.
- Target remains 583,324 / 584,336 B: 1,012 B flash free; static RAM unchanged.

# MerthSloop firmware 2.8.0.56 — tape/sampler refinement

- Consolidated CASST into TAPE, with stronger wow, flutter and wear. Saved CASST loads as TAPE; saved sampler retains its meaning.
- SAMPLER now blends RATE, CRUSH and DRIVE progressively. Removed unconditional internal-bus clipping at low settings; maximum settings retain heavy crunch.
- Target 583,324 / 584,336 B: 1,012 B flash free. Static RAM unchanged.

# MerthSloop firmware 2.8.0.55 — LOFI experiments

- Appended CASST cassette and SMPLR early-digital master characters; old IDs and
  project tag layout retained. Existing buffer/state reused, no new audio ring.
- CASST: stronger flutter/wobble and tape wear. SMPLR: rate hold, signed bit
  quantization and level-compensated input clipping. Selected-mode punch works.
- Four appended editable LOFI presets: DUST KEYS, WORN PAD, TOY BELL, DUST BASS.
  171 factory sounds; old browser IDs, sounds and gains retained.
- Target 583,348 / 584,336 B: 988 B flash free. Low RAM 28,928 B: 568 B free.
  Main RAM/audio pool unchanged. No feature cuts.
- 200 golden renders, unchanged previous 196 hashes; preset health, boot/browser,
  project/section modes, UI, effect mechanisms and 34 native/WASM profiles pass.
  Hardware listening pending. Docs and local installer updated.

# MerthSloop firmware 2.8.0.54 — selected upstream integration complete

- Ordinary POLY overdub keeps overlapping held notes until each note's release.
  Non-POLY overlapping transitions record upstream SF_LEG and reproduce the
  voice mode's envelope handoff without forcing glide. Independent ties remain.
- Factory/user preset arps follow the next preset while untouched; edited arps
  and KBD HOLD remain player-owned. Engine changes compare the prior sound.
- HOME shows the actually running arrangement's themed A–D section badge.
- Protocol 16 carries legato in the existing STEP hi byte (bit 2 value, bit 3
  authoritative). Legacy writes preserve bit 7; aware writes set/clear it.
  Web/preview/native Android snapshots and scene validation retain the marker.
  Android native files write version 2 and continue reading version 1. No new
  project format or larger step: upstream FUN6 and ten-byte steps remain.
- Independently produced upstream 2.8 fixture imports/applies/saves unchanged;
  staggered releases, early/late recording, independent ties, arp ownership,
  protocol round-trips and section badge theme checks pass. All 196 previous
  audio golden renders are unchanged; native/WASM audio and target DSP checks pass.
- Target 582,968 / 584,336 B, leaving 1,368 B flash. Main static RAM 86,176 B;
  low RAM 28,784 B and audio pool 332,876 B unchanged from .53.
- Deliberate MerthSloop UI/content/USB/installer differences remain. This is a
  selective port, not a Git merge of upstream main. FREE TAKE and MIDI import
  duration authoring remain separate follow-on improvements. Hardware listening
  is still needed; no automatic flash or phone installation is performed.

# MerthSloop firmware 2.7.1.53 — instrument preset pack

- Fourteen appended factory sounds: eight NES/Game Boy-inspired CHIP voices and
  VIOLIN, CLARINET, OBOE, BASSOON, HARPSICHORD and OUD under INST.
- Existing synthesis engines and editable parameters; no new sample assets,
  project format or DSP buffers. Original factory browser identities, trims and
  audio fingerprints are unchanged. Combined user Program Change offsets move
  after the expanded factory bank; explicit user bank/slot selection is stable.
- Target 582,424 / 584,336 B: 1,912 B flash free. Low RAM 28,784 / 29,496 B;
  main static RAM and audio pool unchanged. No feature cuts.
- 196 audio golden renders, new instrument range/velocity/release/mix checks,
  canonical browser and boot restoration checks, 32 native/WASM preview profiles,
  trim generation tests and target DSP checks pass. Hardware listening pending.

# MerthSloop firmware 2.7.1.52 — live progression modifiers

- All sixteen white keys repeat progression slots in successively higher octaves,
  including partial final groups. MIDI-range clipping remains bounded/deduplicated.
- Eleven black-key modifiers share ordinary chord-mode pitch/toggle rules, using
  browser ROOT/SCALE even when SEL CHORD is OFF or its keyboard mapping is CHROM.
- HOLD toggles modifiers; momentary release restores them. Sounding key/octave
  snapshots survive draft edits, and unchanged direct/arp gates stay sounding.
- Recording preserves common continuations and retires changed pitches without
  ghost thirds. Shared pending-record skip lists deduplicate pitches; same-step
  common holds do not consume duplicate skip entries before a quantized boundary.
- FX + SELECT claims fade immediately, before the layer-show threshold; quick
  release cannot open an unintended FX page. Other layers retain SELECT tempo.
- Target: 582,048 / 584,336 B, 2,288 B flash free; main static RAM 86,144 / 98,304 B.
  Existing low RAM/audio pool allocations are unchanged. No project format changes.

# MerthSloop firmware 2.7.1.51 — upstream controls

- MENU → FX adds FILTER RESO and PUNCH FADE; FX held + SELECT adjusts fade too.
- Fade covers primary/filter/gate engagement and the temporary LOFI punch boost.
  HALF/DOUBLE remain immediate and saved LOFI amounts are unchanged.
- MENU → SEQ adds STEP ADVANCE (default ON) and DRUM PREVIEW (default OFF).
  Repeat entry with advance OFF closes each transaction; explicit audition remains.
- Noise-drum TUNE shifts noise-filter and enabled high-pass cutoff in the common
  factory/user path, preserving zero tuning and tonal-drum fingerprints.
- Device settings use upstream's canonical packed bits without changing project
  layout, channels, bend, clock output or MRS side settings.
- Target image 580,748 / 584,336 B; 3,588 B flash free; static RAM unchanged.
- Upstream SF_LEG complements independent note continuations; it does not replace
  them. Recording/legato transport integration remains pending.

# MerthSloop firmware 2.7.1.50 — review integration

- Settings restore refuses active/pending transport before touching flash. Side
  records are read back before committing saves; failed writes remain retryable.
- Loader completion requires the exact host success body and verified update-record
  cleanup. Browser/CLI recovery handles generic loader names, rejects ambiguous or
  incompatible destinations and releases failed transfer links.
- Live phone step edits reconcile ratchets. Chord modifier revoicing, held-key
  channel changes and overlapping arp chords retain correct MIDI/pitch ownership.
- MIDI note zero works in mono/legato/unison. Fine tuning reaches rebuilt oscillator
  and sample/grain increments; fine=0 output and sample decode caps are preserved.
  Eight reviewed nonzero-fine golden renders intentionally change.
- SPRING FREEZE/crossfade products use adequate arithmetic width. Stopped LOFI
  re-enable cannot revive a stale synthetic-noise envelope.
- Cold punch initialization, bypassed TRACKS rendering, direct legacy conversion,
  ragged trims and fixed font metrics recover space without removing features or
  adding runtime sample decompression. Combined image: 579,744/584,336 B;
  4,592 B flash free; main static RAM 86,128/98,304 B; low RAM/pool unchanged.
- Android note-send failures retain cleanup ownership; song refresh retires renewal
  tasks. UI request descriptors retain import destinations and staged exports across
  activity recreation; async reviews reject destroyed activities. Accessible timed
  pad/gesture activation uses bounded owned releases.
- Pages verifies deterministic generated assets and preview source provenance.
  Browser regressions use current product names and require their browser dependency.
  The single-file editor explicitly directs audio preview users to the offline PWA.

See [review synthesis](../../docs/reviews/REVIEW-SYNTHESIS.md) and its linked
reports for evidence and hardware/emulator validation limits. No new upstream
filter-resonance/punch-fade controls are included incidentally in this batch.

# MerthSloop firmware 2.7.1.49

- Eleven distinct physical black-key chord modifiers replace octave duplicates.
  Upper keys add sus2, thirteenth, explicit major/minor seventh, downward inversion
  and open voicing; the lower five keep their functions.
- All modifiers work held or as chord-latch toggles, including retained arp chords.
  Suspension and explicit-seventh latch groups are mutually exclusive.
- Four-voice extension priority preserves a requested seventh; thirteenth wins
  over ninth when both cannot fit. Register gestures are excluded from automatic
  voice-leading history to prevent octave drift.
- SEL's six-line physical-key guide shows each position. CHORD OFF and CHROM
  retain the normal scale guide. No project/preset/MIDI parameter IDs change.
- Image 584,336/584,336 (0 free), with unchanged linked RAM.
- Existing chord/scale/arp/voicing/storage/UI regressions and 182 audio goldens pass.
  Normal and UBSan modifier tests cover all keys, held/latch/arp/MIDI ownership and
  3,932,160 bounded output combinations.

# MerthSloop firmware 2.7.1.48

- Restore the normal SEL scale sketch while CHORD is off; show modifiers when on.
- Add MAIN MENU > LIGHTS > knob 4 SCALE LEVEL: DIM/LOW/MID/HIGH (4/8/16/32 us).
  The saved device preference defaults to the original DIM level and preserves
  full played-note lights and unrelated layer/drum indicators.
- Reuse menu toggle and note-move completion code; keep large cold edit handlers
  out of line. Scale scan consumes precomputed timer ticks.

# MerthSloop firmware 2.7.1.47

- SEL's graph area now contains a compact guide to all five physical black-key
  chord modifiers, plus reminders for latch toggling and CHROM root behavior.
- Replaces the former scale sketch; no audio, input, persistence or RAM changes.

# MerthSloop firmware 2.7.1.46

- Live note editing: pitch, length, move, NOTE/HIT ACTIONS and pattern undo/redo
  work during playback. Sounding notes reconcile at the next step; pending
  current-step ratchets are cancelled after structural edits. Active recording
  ownership, voice capacity and pattern-boundary checks remain.
- Progression LIVE can arm during loop playback and tempo recording/count-in.
  Old input owners on that track retire without stopping the other tracks.
- Browser draft edits affect the next pad; navigation/HOME keep LIVE running.
  ARP OFF/ON transfers the held harmony between direct chords and rhythmic output.
  Browser controls take priority over hidden recording-screen knobs.
- Image 584,328/584,336 (8 free); linked RAM unchanged from the preceding build.
- Live-note and progression regressions, UBSan, transport/tie/arp suites,
  20,000-frame UI fuzz, 182 unchanged audio renders and native/WASM agreement pass.

# MerthSloop firmware 2.7.1.45

- Radio BAND reaches a receiver-like response in its first quarter, with further
  low-pass narrowing above that. STATIC has a stronger receiver hiss bed and
  crackling events; DRIFT uses an earlier concave response without a flat upper half.
- Dub DRIVE now adds stronger soft clipping and bass trimming per repeat, with
  an earlier concave response. It reuses CRUSH's mutually exclusive held state.
  Type changes fade through clean and prime the held state to avoid startup clicks.
- Dark reverb has a lower damping range, more feedback at low SIZE and a 6 dB
  compensated wet return; the dry instruments and other reverb models are unchanged.
- Image 584,160/584,336 (176 free); main RAM 89,128/98,304, low RAM
  28,280/29,496, pool 332,876/344,064. No new buffers or linked RAM growth.
- Tests check quarter-range Radio band/static and Dub coloration, all delay
  tap values/transitions, Dark-vs-Hall brightness, tails/bounds/FREEZE/silence,
  arithmetic sanitizers and unchanged normal audio goldens.

# MerthSloop firmware 2.7.1.44

- LOFI adds RADIO: DRIFT/BAND/STATIC, using the existing pitch history and noise
  with a receiver-band filter. OFF remains first/default and retains stored ID4.
- REVERB adds DARK: dense muted stereo tails with SIZE/DAMP, existing FDN buffers.
- DLY TYPE adds DUB: DRIVE/TONE/MIX, with dark filtering on the first repeat and
  feedback. Existing DLY page supplies TIME/FDBK. No new delay buffer.
- Progression PERFORM accepts ARP OFF for sustained direct chords and normal
  recorded ties; ARP ON keeps the current rhythmic path. Both use the common
  emitted-note list/MIDI cleanup. Crossing OFF/ON while live safely disarms.
- Appended mode IDs fit existing storage tags; old modes/OFF and canonical upstream
  default projects remain compatible. New-mode full/section save/load checks pass.
- Image 583,776/584,336 (560 free), main RAM 89,128/98,304, low RAM
  28,280/29,496, pool 332,876/344,064. Combined delta over .43:608 flash/16 main RAM.
- Frequency/decay/bounds/silence/transition tests, direct performance recording,
  sanitizer checks and normal audio goldens pass. Hardware feel remains user testing.

# MerthSloop firmware 2.7.1.43

- SEQUENCES adds a fifth PERFORM page: LIVE / OCTAVE / HOLD / SLOT.
- Local white-key pads select stock/custom progression chords into the existing
  arp. Later pads repeat the progression an octave higher. Changes are NOW-only;
  black modifiers and starter-rhythm output remain deferred.
- Other tracks continue; the owner's stored pattern is preserved and silent.
  REC captures generated arp notes with normal tempo/count-in/quantization;
  the saved FREE preference is unchanged. MIDI input is isolated on the live owner.
- STOP, navigation, load, preset/track changes and panic release ownership.
- Image 583,168/584,336 (1,168 free); main RAM 89,112/98,304,
  low RAM 28,280/29,496; pool 332,876/344,064. No new audio buffers/voices.
- Actual UI/arp/MIDI/recording ownership checks and normal audio regressions pass;
  hardware performance feel remains user validation.

# MerthSloop firmware 2.7.1.42

- SEL adds a third VOICING page: INVERT/SPREAD/VLEAD/STRUM.
- Per-track inversion -3..+3 and CLOSE/OPEN/WIDE spread preserve note count
  and pitch classes. Held/latched notes stay unchanged until the next chord.
- Track settings survive preset changes and project/section/autosave restoration
  without changing upstream P IDs or FUN6 size; neutral upstream fixture stays exact.
- Image 580,652/584,336 bytes (3,684 free), main RAM 89,096/98,304,
  low RAM 28,280/29,496, pool 332,876/344,064. No new audio buffers or voices.
- LOFI punch boosts the selected Vinyl/Tape/VHS/CD character. OFF uses Vinyl
  temporarily; stronger saved amounts are never reduced and release restores them.
- Progression performance is designed and privately prototyped separately; it
  is not included in this release. Hardware audition remains user testing.

# MerthSloop firmware 2.7.1.41

- Shared physical/MIDI step-entry setup and HOME/parameter-card drawing. The
  controlled comparison saves 256 bytes, with unchanged RAM and 116 identical
  framebuffer captures. UI fuzz and entry transaction tests pass.
- Final target image 580,060/584,336 bytes (4,276 free); main static RAM
  89,080/98,304, low RAM 28,280/29,496, pool 332,876/344,064.
- Documentation includes the measured main comparison, duplicated-code audit
  and an upstream optimization shortlist. Hardware boot remains unverified
  for the earlier preset/capacity changes.

# MerthSloop firmware 2.7.1.40

- LOFI order is OFF/VINYL/TAPE/VHS/CD; OFF is first and default. Explicit old
  saved character tags retain their meaning.
- DLY TYPE: DIGI TIME/FDBK/MIX, TAPE DRIVE/TONE/MIX, CRUSH BITS/RATE/MIX.
  Original DLY keeps shared timing, feedback, colour and mix. Inactive character
  parameters save with projects/sections; phone/web archives retain the tags.
- Startup-only factory preset packing saves 2,580 bytes in its controlled
  comparison, using 5,508 bytes spare low RAM. No playback decoding.
- Uses another 2,772 bytes of existing update-region capacity. Target image
  580,312/584,336 bytes (4,024 free); main RAM 89,080/98,304,
  low RAM 28,280/29,496, pool 332,876/344,064.
- 153 preset records match, 182 golden renders remain unchanged, 29 native/WASM
  profiles pass. Target DSP, persistence and UI checks pass. Hardware startup
  timing and SPL boot of the expanded directory still need checking.

# MerthSloop firmware 2.7.1.39

- STEP/NOTES footer: PRESETS: ZOOM / OCT: TOOLS.
- DLY TYPE adds DIGI/TAPE/CRUSH, with TYPE/TIME/FDBK/MIX controls. Original DLY
  retains COLR. Characters save with projects/sections and leave dry audio alone.
- LOFI defaults to OFF, including untagged older saves; explicit saved choices
  still load. OFF retains its amounts and momentary VINYL punch still works.
- Exhaustive tap bounds, maximum-feedback live changes, native UI/storage,
  canonical upstream project, 25 phone archive checks and 25 native/WASM profiles
  pass. Original DIGITAL render bytes and all 182 golden audio renders match.
- Image 580,984 bytes, flash free 580; main static RAM 89,080/98,304.

# MerthSloop firmware 2.7.1.38

- STEP and NOTES now have a chromatic piano-roll grid: black-key shading, C-octave
  labels, beat/bar guides, selected-chain outline and full-height playhead.
  Wide pitch ranges follow the selected note. The graph stays below the cards.
- Turn PRESETS left to zoom in and right to zoom out, through 8/16/32/64 steps.
  Existing note editing and independent sustains remain intact.
- LOFI TYPE adds OFF, smoothly bypassing while retaining the three stored amounts.
  OFF round-trips in projects/sections; older LOFI tags still load. Momentary
  VINYL punch still works over OFF and returns to dry on release.
- A shared UI-only clamp reduces duplicate code; DSP helpers remain inline.
  Target image is 580,708 bytes, leaving 856; main static RAM remains 89,064/98,304.

# MerthSloop firmware 2.7.1.37

- Native STEP zoom, NOTES pitch/length/strength editing, NOTE ACTIONS and drum
  HIT ACTIONS preserve independent note chains, other voices and shared step data.
- Drum FX page 1 exposes level, pan, delay and reverb sends; page 2 exposes the
  working filter. INSERT 2 now exposes blend, type and two parameters.
- Master LOFI offers VINYL, TAPE, VHS and CD characters; mode and amounts save
  with sections/projects. VINYL punch remains temporary and restores settings.
- MIDI expression attenuates synths and drums without altering saved levels.
  Source/channel-isolated bank selection supports factory and real user presets;
  adopting a project clears stale queued program changes.
- Compact descriptors and startup-only table/FIR/FM6 packing retain exact audio
  data. Image: 581,140 bytes; flash free: 424; main static RAM: 89,064/98,304.
- Eleven focused editor/UI/MIDI suites, restored-table and FIR byte oracles,
  USB resampling, synth drums, target budgets and 182 unchanged audio renders pass.
  Hardware startup timing and live interaction remain user validation.
- Wet-color delay/reverb foundations are tested but not connected to audio buses;
  expanded delay models remain next work.

# MerthSloop firmware 2.7.1.36

- Selectively adopts upstream 2.8's early OCTâˆ’ rescue-key power fix, fast-encoder
  transition handling, macOS USB-audio collection order and descriptor version,
  plus averaged/hysteretic external MIDI-clock tempo estimation.
- The installer finds generically named Windows update loaders by handshake,
  prioritizes matching port pairs and allows longer re-enumeration. Existing
  MerthSloop packaging, foreign-loader checks and return-to-stock remain.
- Retains the new reverb types, NEON UI and all MerthSloop editing/performance
  features. This is a selective backport, not a complete SLOOP 2.8 merge.

# MerthSloop firmware 2.7.1.35

- SPRING now uses two unequal dispersive paths with separate pickups for a wider
  stereo tail; TIME remains independent of nominal decay.
- ROOM, PLATE and HALL add three contrasting spaces to the original STEREO.
  They reuse existing delay/AP storage. TIME is active only on SPRING.
- New model metadata retains the project size, default upstream byte identity,
  old spring tags, section recall and deferred FREEZE switching.
- Measured impulse/stereo/decay, overload, switching, persistence and UI tests
  cover every type. Host listening examples are under build/reverb-lab.

# MerthSloop firmware 2.7.1.34

- HOME shows the selected drum kit name instead of the generic DRUM label.
  Kit changes refresh the label immediately, including while stopped.
- Renderer regression covers every kit and compares incremental/full redraw.

# MerthSloop firmware 2.7.1.33

- VINYL's maximum settings are now audibly stronger: slow wow/flutter, darker
  TONE and louder crackle, with exact zero bypass and smooth control transitions.
- VINYL replaces MONO as a held punch-in; releasing restores persistent settings.
  DOUBLE TIME replaces octave down and complements HALF without changing pitch
  or stored BPM. External sync bypasses the two rate gestures.
- SPRING REVERB knob 4 TIME adjusts reflection spacing independently from SIZE's
  nominal decay target. Uses the existing buffer; FREEZE preserves its length.
  Model/time metadata does not overwrite the drum-tuning checksum; default
  projects retain canonical upstream byte identity.
- Image 581,176 bytes, 388 free; main static RAM 85,656, low RAM/pool unchanged.
  Audibility, stereo/release, full-mixer, timing, format and UI regressions pass.
  Hardware listening and deadline measurements for this build remain pending.

# MerthSloop firmware 2.7.1.30

- Explicit section SAVE also updates the existing power-on working-loop object,
  preventing an older root/scale from returning during background autosave's
  20-second rate limit. The named section still holds the snapshot taken at SAVE;
  a queued flush captures the then-current working loop for startup.
- Queued section writes begin once stopped and voices are quiet, without the
  routine settings input-idle delay. Failed startup writes keep the save pending
  and cannot produce a successful-save confirmation.
- Adds C# minor â†’ G Phrygian cold-NOR/restart regressions for stopped/queued saves,
  plus startup-write failure/retry checks. Assertions explicitly remain enabled
  under optimized host compilers. The old behavior fails the new restart test.
- Upstream FUN6 compatibility and project-format tests pass; storage object IDs
  and serialized layout are unchanged. Image 575,160 bytes, 6,404 free, no RAM growth.

# MerthSloop firmware 2.7.1.29

- Polishes settings tabs, long custom-palette names/swatches, About credits and
  complete engine summary values/units. Adds a gutter between header icon and
  track number; About shows the version followed by MERTHSOFT.
- Keeps destination bars within their guide at parameter extrema; abbreviates
  the held-step ratchet card deliberately rather than clipping its word.
- Replaces OCT UP with live OCT DOWN on the same punch-in key. Three-pitch DSP
  tests verify octave-down output and exact dry recovery; all 27 effects pass.
  Transport HALF and simultaneous effects are separate design work.
- Image 575,016 bytes, 6,548 free; RAM, low RAM and pool unchanged. Complete-glyph,
  sanitized UI/fuzz, audio golden, static-analysis and target checks pass.
  No Android APK change or hardware timing claim is included.

# MerthSloop firmware 2.7.1.28

- Withdraws provisional FM6 diagrams; engine pages return to parameter summaries.
- SONG uses shared knob cards, palette-aware section colors, standard header and
  footer. Record-ready/count-in align with the same layout; free takes retain
  their large timer. Disabled controls show muted dashes.
- Consolidates the card primitive/strip, removing the legacy bottom-dial renderer
  and duplicate record header. Help and footer share engine-page titles.
- Generic destination/filter/delay guides start higher, leaving clear space
  between the DLY caption and footer.
- Image 574,856 bytes, 6,708 free; RAM 81,592, low RAM/pool unchanged. Consistency
  changes recover 1,944 image bytes; recording/song interaction and sanitized
  redraw/fuzz tests pass. Physical timing has not been measured.

# MerthSloop firmware 2.7.1.27

- Removes unused right-channel scope capture and volatile note/kick animation
  telemetry from retired visualizers: 1,040 static RAM bytes and 32 image bytes
  recovered. The seven retained captures and all 182 audio goldens are unchanged.
- Fixes a flash-only size assertion in the RAM-only firmware configuration.
- Moves the DLY guide up and reserves space for its complete bottom caption.
- Adds a reproducible target-compiler static-analysis tool and detailed audit.
- Image 576,800 bytes, 4,764 free; static RAM 81,592, low RAM/pool unchanged.
  Target budgets/inlining and sanitized UI regressions pass. Hardware timing
  was not measured. No new Android APK is included.

# MerthSloop firmware 2.7.1.26

- Held SAVE section stores write immediately when stopped. During playback they
  say QUEUED: STOP TO SAVE, then confirm after a successful deferred flash write.
- Explicit section/project saves commit device settings, including 1/8 swing
  mode. Failed section/settings writes remain retryable rather than claiming a
  successful durable save.
- Real NOR-storage regression covers track/drum levels, global swing, cold reload,
  queued snapshot isolation and write failures. Fixes the combined wah/pan LED
  lookup found by the full UI sanitizer regression.
- Image 576,832 bytes, 4,732 free; static RAM 82,632; pool unchanged.

# MerthSloop firmware 2.7.1.25

- Keeps exactly seven visualizers: Oscilloscope, Spectrum, Spectrogram, VU Meters,
  Note Trails, Groove and Song Journey. Selector wraps 1/7 through 7/7.
- Retained saved IDs remain stable; removed IDs fall back to Oscilloscope.
- Removes unused renderers and animation state: 2,912 image bytes and 48 static
  RAM bytes reclaimed. All seven render and settings/navigation regressions pass.

# MerthSloop firmware 2.7.1.24

- HOME's striped sun pulses gently with the transport beat while playing and
  returns to its original brightness when stopped. No extra persistent RAM.
- Beat-phase invalidation works with stationary playheads/long notes; tests
  confirm track-lane/footer pixels remain unchanged by the sun animation.
- Equivalent compact card borders and shorter hints offset most new code:
  579,512 image bytes, 2,052 bytes free; four image bytes above the preceding
  engine/help checkpoint. Target DSP budgets/inlining remain unchanged.
- UBSan renderer and 20,000-frame UI regression pass. Physical redraw cost has
  not been measured. No Android APK update is included.

# MerthSloop firmware 2.7.1.23

- FM6 OPS/PATCH show the active algorithm: six operators, modulation routes,
  carriers and feedback. Patch and algorithm edits invalidate the diagram.
- HOME includes the engine name beside the preset in the selected track color.
- Help puts PRESETS left and ALGORITHM right; hold hints name the actual action,
  including LFO vibrato and ENV tremolo.
- Image 579,508 bytes; flash free 2,056. No static-RAM or audio-pool growth.
  ANALOG engine diagrams are deferred to retain the flash reserve.
- All 32 routing diagrams, dirty/full redraws and patch isolation pass under
  UBSan; the 20,000-frame UI suite and 182 unchanged audio goldens pass.
- Firmware-only update; no new Android APK or hardware timing measurement claimed.

# MerthSloop firmware 2.7.1.22

- Refines HOME's NEON scene with a larger round striped sunset, a warm-to-violet
  gradient and wireframe mountains. Track lanes remain readable underneath.
- Adds 192 image bytes, no additional RAM. Image 578,324 bytes; flash free 3,240.
- Native redraw/palette/state tests pass; physical display timing remains pending.

# MerthSloop firmware 2.7.1.21

- Native UI overhaul: shared knob cards, NEON colors and four-track overview.
- Refreshed held maps, drum/groove and progression browsing, and track-local
  modulation displays; existing gestures and musical behavior are retained.
- Parameter guides and SCREEN â†’ PAGE HELP; existing palettes/custom themes remain.
- 578,132 image bytes, 3,432 flash bytes free, main static RAM 82,680; no additional
  framebuffer or audio-pool allocation. The raw factory sample path remains.
- Real-renderer screen/video captures and host regressions pass. Hardware UI
  latency and LCD contention are pending, not represented as tested.
- No new Android APK is claimed for this firmware-only UI refresh.

# MerthSloop 2.7.1.20

- Restores direct factory sample reads, removing render-time decompression/caches.
- Expands compact lookup tables only at cold startup, before audio starts.
- Removes phone/computer USB audio playback into the FM-1, return controls and
  phone access to musical sequence starters. USB recording/MIDI, native sequences
  and phone access to drum grooves remain.
- Firmware: 578,664 image bytes, 2,900 flash bytes free; main RAM 82,776;
  low RAM 22,772; pool 332,876. Existing project and sample formats are unchanged.

Earlier release checkpoints below retain their original historical evidence.

---

# Latest firmware â€” MerthSloop 2.7.1.19

- Integrates upstream 2.7.1 with canonical song flags, arp IDs and settings. No
  migration of old-fork formats; upstream-produced FUN6 fixture roundtrips exactly.
- Dual-core safe fallback, drum tune/decay, held sample loops/tails, QUANT OFF and
  recording over held notes, arp modes/KBD HOLD, favorites/themes/acceleration,
  MIDI3 remote/clock and web LIVE/song features join existing fork workflows.
- INFO protocol 15: upstream SYN 72â€“76, LIVE 77; companion extensions 85â€“89. Android validation
  follows upstream continuations/held-loop metadata and guards capability discovery.
- Fixes HOLD ownership vs chord latch, duplicate timing undo, SELECT navigation
  through custom starter pages, and persistence of side-record-only preferences.
- Lossless factory sample packing saves 33,744 bytes net. Image 559,992 bytes,
  static RAM 74,888, low RAM 22,772, pool 333,308. All 182 renders match the raw build.
- Host/source checks pass as recorded in the integration guide. Physical hardware
  checks remain pending. The updated ARM64 APK is built.

## Previous checkpoint

# Latest firmware â€” MerthSloop 2.6.0.18

- HOME menu â†’ TIMING â†’ SWING GRID selects STEP (existing behavior) or 1/8.
  TRACKS shows SWING8 in eighth mode. HALF TIME hats and eighth-note arps can
  shuffle on a 1/16 pattern. The setting survives power-off without a project change.
- Preview, recording snap, straight SLICER rates and GROOVE markers follow the grid.
  Eighth-mode nudged events/gates/ratchets use their own step width. Grid lines no
  longer overwrite shifted markers. Live ARP retains its own ARP 2 swing.
- Compact aligned ROM groove masks retain every factory hit. Full image 581,360
  bytes (204 free); static RAM 82,116 and pool 332,876 bytes are unchanged.
- Actual mixer tests check HALF TIME at 0/375/500/875ms, legacy compatibility,
  uneven loop and early/late FEEL preview parity, settings and visualizer pixels.
- Local/origin main mirror upstream 2.7.1; feature-branch integration remains
  separate. The assessment records flag, arp-ID, settings and memory conflicts.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.17

- Native CUSTOM progressions: count 1â€“8, editable degree per position and 4/8/16-step
  span within the 64-step pattern. Shared rhythm/key/voicing/mode controls, preview,
  ADD/REPLACE and undo. Applied notes save with the song; draft lasts until power-off.
- Shared sequence rendering and compact ROM degrees fit the new page. A 100,000-event
  comparison preserves factory starter output. Custom count/span/rhythm/mode,
  preview, undo and navigation tests pass, alongside the existing UI fuzz and ties.
- Swing scheduling and visualizer regressions verify 0/50/100 on odd sequencer
  steps. No swing behavior has been changed pending the reported setup details.
- Target 581,560 bytes (4 free); static RAM 82,116 and pool 332,876 bytes.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.16

- STEP renders all tied chord voices, including at bank boundaries. Independent
  continuations connect without gaps; stale rest/tie payload cannot distort range.
- Redraw caching tracks all four pitches. STEP knob zoom no longer covers notes.
- NOTE and LEN share the anchored source (SRC/SEL). Shortening past the cursor
  no longer makes subsequent LEN edits target an empty destination step.
- Added pixel-level regression checks and anchored-length control tests. Playback,
  editing, move/merge and the 20,000-frame UI fuzz pass. Hardware audition pending.
- Image 581,512 bytes (52 free); static RAM and pool unchanged from the tie release.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.15

- Independent note sustains can cross later note onsets. Hold OCT+ and scrub STEP
  with knob 1; knob 4 chooses ALL or one source pitch. LEN can shorten that
  selection, and EDIT deletes it without erasing the other voices. Use POLY.
- Legacy ties and saved project layout remain compatible. Single-event move and
  transpose are guarded for independently tied patterns; whole-pattern transpose
  remains available. Live recording and phone/web duration editing are follow-on work.
- Instrument browsing preserves voice mode (including LEGATO), glide, priority and
  allocation alongside track effects. Explicit sound initialization retains defaults.
- Lossless shared icon rows and measured UI code reductions fit the additions:
  target image 581,448 bytes (116 free), static RAM 82,116 bytes. No audio-path
  icon decoding or new pattern buffers.
- Native playback/editing, voice identity, MIDI balance, project round-trip,
  icon pixel equivalence and browser/native audio regressions pass.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.14

- Wah uses a dedicated, narrower resonant band-pass response: it emphasizes a moving
  vocal-range band and rejects lows/highs instead of only sweeping brightness.
- Knob 3 is CENTER; the upper sweep stays in the useful wah range. Makeup gain is
  controlled. LFO and envelope modes use the new response; ordinary filters are unchanged.
- Real DSP tests check low/high rejection, movement of the resonant peak, output
  bounds and ordinary-filter golden compatibility. No new runtime buffers.
- About retains name, version and attribution; its decorative sail is removed to
  fit the instrument feature. Target image 581,512 bytes (52 free).

## Previous integrated checkpoint â€” MerthSloop 2.6.0.13

- Auto-pan uses LFO+FX; ENV+FX remains auto-wah.
- Ordinary keyboard notes support overlap handoff after an octave switch without
  a new attack. The note ends only after its last physical key releases.
- Held pitches fully light their mapped key in the current octave even when NOTES
  display is disabled. Other tracks and generated chord/arp behavior remain independent.
- Tests cover both octave directions, both release orders, balanced MIDI output,
  unchanged voice age, final release and a deliberate fresh attack.
- Target image 581,548 bytes; static RAM and pool unchanged.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.12

- Envelope wah responds to typical synth levels: sensitivity gain is corrected and
  defaults favor a pronounced attack/release sweep.
- Wah now uses a resonant low-pass with a useful dark-to-bright cutoff range,
  retaining resonance at the open end. Ordinary track/global filters are unchanged.
- DSP response tests verify frequency sweep, resonant emphasis and sensitivity;
  all 182 inactive-effect golden renders remain unchanged.
- Target image 581,276 bytes (288 free); static RAM and pool unchanged.

## Previous integrated checkpoint â€” MerthSloop 2.6.0.11

- ENV+FX: selected-synth auto-wah, free/beat-synced LFO or own-signal envelope follower.
- SEL+FX: selected-synth auto-pan, waveform/depth and beat sync.
- Both preserve keyboard/scale lights, support HOME locking and clear on release,
  STOP/panic or track change. They do not overwrite saved patch parameters.
- PR 111 independent memory adaptations: packed MIDI owners, DRAWBAR arena state,
  shared main-loop/OTA scratch and flash XIP preset banks. Dual-core is deferred.
- STEP OCT+/OCTâˆ’ help persists while held, with transient warning priority.
- MerthSloop startup, About, installer/editor and Android labels. Firmware uses
  upstream major.minor.revision plus independent build number, now 2.6.0.11.
- Target: image 581,420 bytes, static RAM 82,100 bytes, pool 332,876 bytes.

## Previous integrated checkpoint â€” 2.6 Merthsoft.10

- Hold LFO or ENV and tap OCT+ to apply vibrato/tremolo parameters to the track's
  saved shared LFO. Rate, waveform and beat division transfer; tremolo depth is
  exact and vibrato depth rounds to the regular pitch destination's steps. The
  temporary effect stops to avoid doubling; unrelated destination depths stay.
- LOFI MOTN ARP retains OFF/OCT/MAJ/MIN and adds every SEL chord shape, using the
  existing harmony builder and root/scale. No additional voices or persistent RAM.
- Identical font bitmaps share offsets: 816 bytes less data, unchanged renderer
  and exact pixels for every supported character.

- STEP move brush now merges a moved onset into an existing chord (four-note
  limit, no duplicate pitches). Destination timing/dynamics/locks remain; a single
  source note takes its duration, or tied events require matching lengths. Source
  locks and overflow reject before editing. Complete undo/redo is retained.
- Duplicate LOFI MAJOR/MINOR menu entries are removed; MAJ/MIN already cover them.
  Exact suffix/prefix glyph sharing saves a further 608 bytes, with no renderer change.

- Note Trails excludes released-voice/delay-tail continuation and cuts the current
  synth/drum column during the fully wet closed master GATE phase. Partial wet
  retains the dry path; pending closed-phase hits are not replayed on reopening.

- GLO SYSTEM always shows CPU percentage, including standalone use without USB.
  The header still indicates a configured USB connection.
- Note Trails sustained notes now draw continuously across time columns; overlap
  color stripes remain visible. The earlier narrow strokes caused artificial gaps.
- ADD de-duplicates existing pitches as well as incoming pitches and preserves
  tie slide flags. Both Apply gestures, undo/redo and 1,000 independent randomized
  pitch-set cases pass; late overflow is atomic.
- Native SEQUENCES: ALGORITHM selects ADD/REPLACE. ADD retains current timing,
  locks and fill, merges tied notes, de-duplicates pitches, and rejects fifth-note
  overlaps atomically. Preview remains the starter alone; hardware undo/redo works.
- Note Trails captures short attacks across both UI frames, distinguishes coincident
  track pitches, keeps the cursor clear of notes, and uses white-on-black drum hits.
- Instrument changes preserve track effects across engine/preset/user-preset browsing
  and MIDI Program Change. Both OCT buttons on a normal FX page reset that track's
  effects. INIT SOUND and song loading retain their intentional reset/load behavior.
- Factory base records are bit-packed losslessly: 968 bytes less data, 64 extra
  decoder bytes and a 21-byte table; the measured net target saving is 880 bytes.

Upstream SLOOP 2.6 is integrated into merthsloop. Local main and origin/main
match upstream revision 8fad1a1. Five follow-up web commits add firmware WASM audio
preview, inline sequence editing, sample gain/level matching and offline/PWA support.
The browser adapter uses the fork's multi-note arp cleanup and firmware memory routines;
all twelve engines match the native reference sample-for-sample.

- Upstream's 27 punch effects replace the former black-key FX modifiers. Remote
  performance negotiates sixteen or twenty-seven effects with the connected firmware.
- Track inserts, synced envelopes/LFOs, filter modes, texture, drum mutes,
  note length/velocity, MIDI sustain/bend/routing/learn and step entry are integrated.
- Native groove/progression libraries, independent starter rhythms, sequenced arps,
  physical latch gestures, scale guides/lights, editing tools, held/locked modulation,
  complete song-load undo and the Android/editor protocols are retained.
- FUN6 imports older projects and shifts engine locks correctly. Web backups include
  MIDI object 10. Android negotiates 61/53 or 71/63 parameter layouts, configured
  channels including OFF, and correctly routes optional bass.
- Factory drum definitions are losslessly shared; all 512 expanded sounds match.
  Startup uses a text title instead of the bitmap logo. Size compilation preserves
  hot DSP inlining, guarded against newly outlined per-sample helper calls.

Target image **581,060 bytes** (504 free); static RAM **94,500/98,304**
(3,804 free); pool **333,900/344,064** (10,164 free). Fully overwritten
scratch uses 8,800 bytes beside the flash driver, leaving 12,820 bytes there;
linker/build checks preserve an 8 KiB driver reserve. Stack guards are unchanged.
HAL and call-free RAM driver checks pass.

Package **609,941 bytes**, identity **FM-1_900**. SHA-256:
`5120a513bf70ad40e0b881983f3346367fa766d5334948258922b1067e11c0cf`.
Installer: http://127.0.0.1:8815/webapp/installer/

Host functional, serialization, UI, DSP and divide-by-zero checks pass. All 182
reviewed audio renders match, with zero health, voice/routing or crash failures.
The one LOFI arp fixture change preserves source velocity; forcing velocity 100
alone reproduces the exact upstream fixture. Android builds with zero warnings/errors;
web and companion domain tests pass. Hardware flashing/playback was not performed
in this integration session. Static instruction/call checks are not cycle measurements.

Unmarked settings words are interpreted as old fork settings. Upstream's old
channel-filter bits overlap fork visualizer/scale flags, so imported upstream
settings may need channel-filter re-selection. New settings have a format marker.
MIDI configuration is device-wide, not saved independently with each song.

The earlier intermittent song-save report remains unresolved. NOR round-trip tests
pass; no claim is made that its hardware cause has been fixed. A pre-existing
settings-write failure also lacks automatic retry; it remains follow-up work.

## Android chord labels

Quick sampler workflows and the named native FM1 song library are integrated.
Song loading retains a recovery archive before mutation, restores shared sound
dependencies and arrangement, then verifies objects and edited FM6 track voices.
Physical FM1 round-trip remains pending; see [song library](FM1-SONG-LIBRARY.md).

- Perform chord pads and mapping choices derive Roman numeral case and diminished/augmented
  marks from the selected scale, matching the default diatonic chords they play.
- Workstation tests cover all eight degrees in major, natural/harmonic minor, both
  pentatonic parent harmonies, and the differing Dorian/Mixolydian chord qualities.

## Android expanded chord mode

- Thickness adds one or two upper octave copies after base voice leading, with
  duplicate/out-of-range notes omitted. Optional bass remains unchanged.
- Pad range offers 8/15/22 pads for direct access to higher octave degrees. Higher
  pads retain explicit register and scale-aware labels; rows stay playable in size.
- Saved performance presets retain thickness/range and older presets remain compatible.
- Voice leading passed 26,880 transition checks; multi-pad latch stacking remains future work.

## Android Part writing

- Voice lead is renamed Smooth voicing. A separate Part writing checkbox takes
  precedence and searches wider base-chord voicings for fewer parallel perfect
  intervals, appropriate tendency-tone resolution and independent voice movement.
- Selected root/register and explicit inversions remain authoritative. Voice-count
  changes fall back to smoothing; optional routed bass participates in rule scoring.
- Thickness remains octave doubling after base voicing. Touch/controller chord
  histories are separate. Saved presets retain the option; old presets keep it off.
- This is a local preference mode, not a strict SATB/harmonic-rewriting engine.
