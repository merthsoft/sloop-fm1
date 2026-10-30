# VOICE / master LOFI dropout investigation

Read-only investigation of the frozen actual .92 source. Production sources,
version, package, generated files and Git state were not changed. Private work:
`build/voice-lofi-investigation/voice/`; immutable input:
`build/voice-lofi-investigation/baseline/`.

## Finding

No VOICE DSP, envelope, vowel-morph or ownership corruption has been reproduced
when master LOFI changes. The actual mixer produces identical **pre-master audio**,
all synth `voice_t` bytes and VOICE breath RNG state with LOFI OFF and with each
LOFI character enabled. This excludes a deterministic voice-state change in the
tested controls/scenes; it does not exclude a device deadline failure or spectral
masking after the master processor. No timing conclusion is drawn from host runs.

The present oracle deliberately does not call the hardware overload callback.
That callback is an external cause of voice loss; removing it from this oracle
isolates whether the engine and effects themselves change note ownership.

## State and ownership trace

- `eng_formant.c` has no reference to master LOFI settings, its processing state,
  admission budget or CPU-shedding policy. Four formants use this voice's
  `s[0..6]`, `ph[0..2]`; `s[7]` packs TALK progress and the note's random vowel.
  `formant_note_on` resets those states only on note startup/retrigger policy.
- VOICE's only engine-global mutable DSP state is `formant_nz`, the breath RNG.
  Master LOFI uses **different** `vinyl.rnd`, ring and filter histories.
  `units_split` keeps multiple VOICE parts together on core 0 to preserve their
  single RNG sequence. A single VOICE part may be sent to the worker.
- `mix_part` stores temporary MORPH parameters in existing per-core `pnew`
  scratch. It renders the same track's overlay then restores base parameters;
  the master processor runs after both workers have joined. MORPH endpoint
  values, per-voice phases and envelopes are not shared with the master effect.
- Snapshot LFO-timing changes keep a part on core 0. A vowel-only A/B change
  leaves that timing eligible for the ordinary worker route.
- VOICE's hard local cap is **four**, GRAIN's is **three**, regardless of the
  expanded shared admission selector. A fifth held VOICE pitch necessarily
  replaces an existing local voice. In .92, replacement of a still-gated
  different POLY pitch **does increment `voice_kills`** in `voice_start`.
  Reuse of an already-released tail does not meet that condition.
  The oracle therefore asserts the expected voices/gates on every block.

A secondary scheduling concern is that `units_split` counts every track assigned
VOICE when deciding the shared-RNG exclusion, including a track with no sounding
voices. Thus an empty second VOICE track can pin a busy VOICE part to core 0.
This is a real conservative exclusion, not a demonstrated cause in the user's
previous GRAIN + VOICE + LOFI-engine layout. A safe change needs to account for
notes started in `events_block`, preset fades and state resets in the current
block; using only the previous render count would be insufficient evidence.

## Actual-code oracle

`source/tests/voice_isolation.c` includes the copied production `hostsim.c` and
all engine, voice, sequencer and effect code. Its only copied-code instrumentation
is a callback immediately before and after `vinyl_process_mode` in private
`fx.c`. No renderer or ownership algorithm is substituted.

Each case starts three held Flute Dust/GRAIN pitches (60,64,68) and four held
VOICE pitches (48,52,56,60), with maximum shared admission. It overrides factory
mono mode/attack/sustain to isolate held polyphonic ownership; the other engine
parameters still come from each actual factory VOICE preset. Optional DRIFT
morphs VOICE VOWL 0 to127 with otherwise matching A/B captures. It renders 3,500
32-frame blocks (about2.54seconds). Master LOFI is OFF for500blocks, then the
selected character, with its three controls at64. Each character gets a fresh
process. The host runs `CORES=2 CHAOS=7`; the worker callback really renders in a
POSIX thread with perturbed scheduling.

For every block:

1. Hash pre-master stereo samples and full synth voice structs plus VOICE RNG.
2. Copy every synth voice before master LOFI and assert exact equality afterward.
3. Assert all three GRAIN and four VOICE slots stay active and gated.
4. Require no global voice kills or CPU sheds during the run.

`run_isolation.py` compares each preset/MORPH pair's six enabled characters
against its OFF reference. Output/pre-master state differs between presets or
MORPH settings as expected, but must match across all master settings within a
pair. This is stronger than checking the final master waveform or voice count.

Initial matrix without drum hits: **98/98 cases pass** (seven factory VOICE
presets x seven LOFI menu settings x two MORPH states), 343,000 actual mixer
blocks. Evidence: `isolation-no-drums.log`. The full matrix including kick, snare
and hat hits and selected sanitizer follow-ups are retained separately; see the
completion entry below.

Compiler: pinned Zig0.13 host frontend through `build/host/optimization-cc`,
`-UNDEBUG -O2 -fwrapv -fno-strict-aliasing`, generated headers from the frozen
snapshot. UBSan follow-ups additionally use `-fsanitize=undefined
-fno-sanitize=shift -fno-sanitize-recover=all`; shift sanitizer is excluded because
the firmware intentionally uses signed DSP shifts outside portable ISO rules.

## Target-code inspection

The immutable pinned JieLi disassembly shows `formant_render` reserving372local
stack bytes plus its register-save frame. The four cascaded resonators execute
12 native signed 32x32-to64 MACs per output sample; coefficient values also
spill/reload in the inner loop. Setup recomputes four coefficient triplets and
voice-dependent pitch/vowel quantities every control block, plus two source
phase reciprocals. `blep` can execute a hardware divide in its edge region.
These are workload facts, **not measured cycles** or proof of the dropout cause.
No evidence-backed DSP fix was made merely to reduce these counts.

Changing coefficient caching or resonance arithmetic without a complete key
(including effective pitch, envelope/LFO cutoff, TALK/RAND, vowel endpoints,
SHIFT, bandwidth/Q and velocity/shape source controls) risks freezing modulation
or changing sound. Such caching is not necessary to establish the ownership
result and was not applied speculatively.

## Recommended synthesis with the other investigations

Use the mixer/deadline specialist's reproducible overload and worker evidence to
explain actual voice deletion, and the master specialist's post-master tests to
separate masking from deletion. Our pre-master oracle provides a clean control:
turning LOFI on does not, by itself, change VOICE/GRAIN gates, stored oscillators,
formant histories or morphed parameters. A fix must preserve that contract.

Do not increase voice admission again or weaken overload protection on this
result alone. It establishes what is unchanged and where to concentrate, rather
than claiming the device problem is fixed.

## Completion evidence

- `isolation-no-drums.log`:98/98 cases pass, 343,000 mixer blocks.
- `isolation-drums.log`:98/98 cases pass with actual `drum_on` kick/snare/hat
  hits, another343,000 mixer blocks. All pre-master and full synth-state hashes
  equal their corresponding OFF reference; no lost expected gates or kills.
- `isolation-ubsan.log`:14 additional correctly supplied scenarios pass under
  UBSan (CHOIR AAH and VOX PAD, all seven master settings, vowel MORPH enabled,
  same drums and chaotic worker). Initial shell-loop launcher expanded away its
  arguments before reaching Linux; it was replaced by `run_ubsan.py`. No result
  from that malformed launch is counted as a firmware failure or a passing case.
- `cap-off.log` and `cap-tape.log`:adding pitch67 after four held VOICE pitches
  yields48,67,56,60 in both cases, four active voices and exactlyone held-note
  replacement counted. This independently confirms the local-cap behavior and
  corrects the initial investigation hypothesis that it would not be counted.

No production patch is proposed from this scope: the matrix rules out the
ordinary state-corruption hypothesis in the exercised cases, while hardware
wall time and post-master masking require the other specialists' evidence.

## Independent review of subsequent dropout fixes

Read-only review of the parent's in-progress measured unit scheduling and
CPU-only owned-note readmission candidates. These are newer than the immutable
.92 matrix above; the matrix must not be described as testing the later fixes.

Two issues were found and sent directly to the implementation specialist:

- CPU pending flags could remain after the last pending pitch was cleared,
  causing a128-pitch scan on every control block indefinitely. The candidate
  now clears the part flag after an exhausted scan with no valid pending notes.
  Private physical-key recovery, vanished-owner and last-marker cleanup checks
  pass in `voice/review/ownership_review.log`.
- The natural-lifetime exclusion temporarily relied on `engine->amp != NULL`
  to exclude SAMPLE. Its actual engine initializer has a null amp callback;
  independent compilation of `cpu_shed_recovery_test` caught the resulting
  failed SAMPLE no-pending assertion. Explicit SAMPLE/SLICE exclusion remains
  necessary alongside the callback/PHYS/zero-sustain rules.

The scheduler's current-active FORMANT check is safe with respect to the shared
breath RNG: events have already been applied before partitioning, and every
part with an active FORMANT voice remains together when there is more thanone.
Released and fading voices count. An inactive second FORMANT part does not
advance that RNG in the render. Measured cells have one writer per render unit
and are read for the next block only after the worker joins. No new unguarded
shared RNG access was found.

`unit_scheduler_test` independently passes identity/fresh-note invalidation,
manual-mask reset, timer wrap/outlier and current FORMANT activity cases, as does
its first-worker-job timeout mode. Hysteresis subtraction cannot underflow for
a legal previous mask: the selected peak is the minimum over all legal masks,
including that previous mask. Costs and voice-scaling products stay below32-bit
limits under the imposed per-unit duration bounds. These tests establish policy
and visibility contracts, not FM-1 execution time or a hardware dropout cure.

Review completion: the explicit `!eng_sampled(e)` exclusion was restored and the
independent full actual-ISR recovery suite passes with `CORES=2 CHAOS=7`.
Evidence: `voice/review/cpu_shed_review.log`. The candidate also invalidates
pending ordinary notes when sustain becomeszero in `engine_block`, closing the
control-change corner case identified during review. No unresolved actionable
ownership, RNG or partition arithmetic defect was found in the reviewed
candidate. Target build/stack/resource checks and hardware timing remain the
parent integration's responsibility.
