# Track sound snapshots and morphing

The native EDIT → MORPH page keeps two parameter states, A and B, for each
synth track. It is a performance tool for changing the sound of an existing
part while its notes keep playing. It does not capture or replace a sequence.

## Controls

- Knob 1 MORPH: 0% is A, 100% is B. Continuous parameters interpolate in their
  native control units; named choices switch at the midpoint.
- Knob 2 SET A: turn right twice to capture the current sound as A.
- Knob 3 SET B: turn right twice to capture the current sound as B.
- Knob 4 SCOPE: ALL, SOUND, FILTER, ENV, LFO or MIX. Captures always retain the full sound; scope only limits what is applied.

The next EDIT page is **MORPH GO**:

- Knob 1 MOVE: STOP, A>B or B>A. Select a direction to start from that endpoint. Completion returns MOVE to STOP and retains the destination sound.
- Knob 2 TIME: the same eight second-based and eleven tempo durations used by DRIFT/DRONE.
- Knob 3 MORPH: live position; turning it cancels a transition and takes manual control.
- Column 4 STORED: EMPTY, A, B or A+B; read-only.

Timed transitions work while stopped. Tempo durations pause with transport and resume without a clock-reset jump. Changing TIME preserves the completed fraction and uses the new timing for the remaining fraction. STOP freezes the current sound. Capturing A/B, loading a sound, or applying a project/section cancels the old run. A sound-browser audition pauses it; cancelling the audition restores its sound, controls and progress. Starting a one-shot turns off DRIFT→MORPH on that track so the two do not fight; other DRIFT targets remain independent.

Capture A, edit the sound on the other pages, capture B, then turn MORPH.
Each track keeps its own pair when switching tracks or leaving the page.
Both endpoints are required before morphing. Capture and morph work during
playback and recording without disarming the track.

![Native track morph page](../../assets/screens/merthsoft/neon/morph-scope.png)

## Selective scope

| Scope | Controls applied |
| --- | --- |
| ALL | The complete supported capture below |
| SOUND | Eight engine controls, glide amount and unison detune |
| FILTER | Track DJ filter and ANALOG filter type; also ANALOG cutoff/resonance/key tracking and LOFI TONE |
| ENV | ADSR, envelope modulation destinations and ENV SYNC |
| LFO | Rate, waveform, phase, fade, destinations, sync, trigger and polarity |
| MIX | Level, preset trim, pan, distortion/chorus/delay/reverb sends, and track insert controls |

SOUND includes native engine filter controls; FILTER provides a narrower way to animate those supported controls. Other engines still have the common track filter. Scope applies to manual, DRIFT-driven and one-shot morphing. Unselected controls keep their current values, including levels and sends when you animate SOUND. Switching scope does not undo earlier changes.

![Native scoped morph controls](../../assets/screens/merthsoft/neon/morph-scope.png)

![Native one-shot morph controls](../../assets/screens/merthsoft/neon/morph-go.png)

## Scope and lifetime

Captured controls are level, envelope and its modulation destinations/trim,
LFO and its destinations/sync, eight engine values, distortion/chorus/delay/reverb
sends, glide amount, pan, unison detune, track filter, insert and ANALOG filter type.
The canonical track descriptors classify common controls; in particular envelope
times remain numeric even when ENV SYNC displays a list of divisions. Engine
selectors are visited before their dependent fields, whose dynamic labels can
identify named controls such as the LOFI wave table or PHYS chord choice.

Pattern notes/ties/locks, timing, root/scale, transpose, chord/arp routing, mute,
voice mode and allocation are excluded. Global effects, live punch settings,
DRIFT/EVOLVE/SUSTAIN runtime settings, uploaded audio and edited FM6 operator
payloads are not duplicated. An engine's existing sample/patch selector is an
ordinary discrete engine control, using its existing loader/polling behavior.
This is parameter interpolation, not an audio crossfade between two engines;
discrete changes can be abrupt.

Loading a factory/user sound invalidates the pair. STOP and track selection do
not. Section SAVE persists A/B, readiness, manual blend, scope and duration alongside the current
sound; loading that section restores its pair, with MOVE stopped. Working-project autosave also
checks A/B-only changes, after the existing stopped/idle gates. Queued live saves
remain RAM-only until STOP flushes them. DRIFT can animate the pair at runtime;
its runtime mode is not made persistent by this extension.

## Execution and locks

Manual capture/apply runs in the UI's existing IRQ exclusion, never in a sample loop.
Every explicit knob move computes
from immutable endpoints, so scrubbing back and forth does not accumulate rounding
error. Unchanged endpoints reproduce exact parameter values.

For a currently active parameter lock, capture reads its underlying base if the
instantaneous value is still the lock's value. Morph updates that base while
leaving the lock audible. Leaving the locked step restores the new morphed base.
A knob override follows the existing lock ownership test. Voice state, gates,
MIDI output, RNG and pattern memory are untouched by the operation.

## Space and validation

The pair uses 852 bytes for three tracks × two complete 71-value arrays, plus
42 bytes of per-track controls and 36 bytes of one-shot phase state. Only the documented subset is applied. Target
alignment/global layout determines final occupied RAM.

To fund the feature, ANALOG's previously duplicated LP and BP/HP bodies now share
one out-of-line renderer. Its per-sample helpers remain inline. The target budget
now follows `analog_body` with the original 638 weighted-instruction allowance;
the measured shared loop scores 640 and has zero loop calls. The tiny dispatcher
is not treated as evidence of a speed improvement. Hardware deadline/stack
qualification remains required; host timings do not establish target cycles.

`tests/track_snapshot_test.c` exercises all engines at every morph position,
exact endpoints, dynamic/named controls, missing endpoints, locked bases,
voice/pattern/MIDI/RNG preservation, invalidation, isolation and native controls.
Audio goldens, engine/filter coverage, dual-core/timeout parity and the full UI
navigation/fuzz suite supplement these checks.

## Automatic DRIFT target

DRIFT TARGET MORPH shares DRONE's eight timed and eleven transport-synced cycles.
DRIFT MODE selects RAND, SIN, TRI, SAW or SQR using the [shared modulation evaluator](MODULATION-WAVEFORMS.md); MORPH does not duplicate waveform or clock logic.
The manual position is the center; maximum amount permits approximately ±64 of
127 position units, clamped to A/B. The MORPH readout shows the effective position.
Synced cycles pause on STOP; timed cycles continue. RAND mid-cycle sync entry freezes
the current value until the next boundary, while repeating shapes follow transport phase. Each track retains its private RNG.

The mixer computes a temporary parameter overlay once per control block, using
its existing per-core preset-fade scratch buffer. It restores the base after
rendering, including sends and inserts. No extra static RAM is allocated. Active
locks override automatic motion; their saved bases remain unchanged. Saves retain
the base sound. A/B captures remain immutable, and no MIDI or gate events are emitted.
Manual and automatic morphing share snapshot.c's parameter and interpolation rules.

Automatic morph requires both captures. SAMPLE/GRAIN source, FM6 patch and PHYS
model must match between A/B and the current base, because those selections also
control allocation/loading outside rendering. Incompatible pairs show MATCH SOURCES.
Preset crossfade rendering takes precedence over the temporary overlay.

MORPH tracks whose A/B LFO rate or sync differs from the current base remain on
core 0, including target-change fades: worker prediction cannot anticipate their
global RNG wraps. Timing-stable morphs, including vowel-only changes, use the
ordinary worker eligibility rules. Unchanged overlay values skip descriptor and
lock lookup; equal endpoints skip interpolation division. Hardware deadline
qualification is still required. drift_morph_test covers overlay/lock/source rules,
all timing choices, private RNG and dispatch exclusion; mix_split_test adds synced
morph/LFO coverage across core masks, chaotic scheduling and worker timeout.


## Persistent A/B storage

The existing settings A/B sectors have an optional fork trailer at byte 2048.
No partition or upstream payload length changes: existing settings/SYN/MIDI
bytes retain their format, and samples and the SDK key/MAC sector are untouched.
The 1,348-byte bank contains a magic, four section records, one working record
and a CRC. Each record stores the associated project CRC, three readiness/blend
pairs and two signed-byte sets of the 43 captured controls per synth track.
Tests verify that the captured descriptor ranges fit signed bytes.

A settings write programs and verifies this trailer before publishing its
commit header. A failed write retains the previous complete settings copy and
retries. Project and settings commits are separate: if interrupted between them,
a project CRC mismatch discards stale captures instead of applying them to a
different sound. Missing trailers from upstream writes are treated as empty.
Upstream-format backups do not carry this fork trailer.

Section records occupy 1,072 bytes of pool RAM; staging reuses project scratch.
Startup literal packing now allows 5,500 bytes in main RAM (pool limit 2,500),
funding persistence without adding any audio-path decoding. Current .85 image:
584,268 bytes, 68 flash free; main 97,324/98,304, low 28,928/29,496, pool
335,424/344,064, leaving 448 bytes beyond the existing 8,192-byte pool reserve.

`snapshot_persistence_test.c` exercises cold restart, saved/queued sections,
A/B-only autosave, unchanged upstream prefix, trailer write/drop/corruption
failures, retry, absent trailers and mismatched project rejection. Hardware
power-cycle verification remains required.

## One-shot scheduling and saved metadata

Each part owns a phase accumulator. The existing motion timing helpers supply free increments or transport phase; MORPH does not add another waveform evaluator or beat clock. Interpolation runs only when the displayed 0–127 position changes, not every sample or every block of a slow sweep. Idle parts skip the transition helper. Preset crossfades pause progression until their engine is ready. Parameter locks retain their instantaneous values and receive the new morphed base underneath.

The dispatcher applies the existing core-0 exclusion when a running transition can alter LFO timing. A scope that excludes LFO can use normal splitting. Automatic SOUND/ALL transitions retain the existing matching source/patch requirement for SAMPLE, GRAIN, FM6 and PHYS; the other scopes do not change those source identities. Named controls switch at the midpoint; this remains parameter interpolation rather than a crossfade between separate engines or sample sources.

Scope/time use seven spare bits in each existing snapshot trailer track record: six scopes × nineteen durations fit 114 states. Readiness retains two bits and position seven. Zero metadata in an older record decodes as ALL / 1S. The record remains 268 bytes, the bank 1,348 bytes, with the same upstream settings prefix and no new flash region. A running gesture is not resumed automatically after loading or power-on.

`morph_transition_test` covers all scopes and engines, category isolation, all durations in both directions, endpoint hold, stop/restart, manual/capture takeover, source safety, LFO dispatch and actual page/knob routing. `snapshot_persistence_test` checks every scope/time/readiness combination at both endpoint positions, old zero metadata, real cold restart, section saves and write-failure retry. Existing DRIFT/mixer parity, audition restoration and full UI fuzz remain in the validation set.
