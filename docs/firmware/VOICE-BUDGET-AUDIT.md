# Voice budget audit — isolated measurements, 2026-10-25

The initial audit below is historical. Weighted admission and FM6 eight are now
integrated; the optional 10 / 15 through 16 / 24 settings and drone recovery are documented in
[the current experiment](VOICE-BUDGET-EXPERIMENT.md). Device qualification remains open.

## Recommendation and status

Do not raise `NVOICE` globally. The best candidates for hardware qualification are:

1. **Weighted admission using the existing eight slots per part:** allow up to twelve LOFI/WHEEL voices across tracks, while retaining an eight-voice allowance for other engines. Keep all existing engine caps and unison behavior. The isolated prototype passed golden audio, allocation, ownership, arena and two-core checks. Its weights are hypotheses, not calibrated AC7911 costs.
2. **FM6 cap six → eight, retaining the shared eight-voice budget:** this fits the existing engine arena without increasing linked flash or static RAM. Audio, FM6 bank/envelope tests and arena tests passed. It increases worst-case single-track work, so hardware qualification remains mandatory.

Neither candidate is installed in production sources. Do not combine them for release without repeating qualification on the final sustain implementation. No instrument content, sample encoding, engine IDs, saved parameter IDs or project format was changed. No version bump, firmware packaging, installer generation, serving, commit or push was performed.

## Reproducible scope

Branch: `merthsloop`; HEAD at capture: `76eae6d6322cce4b75e3802d1ac565f191d431c5`.

The actual dirty checkout was copied into ignored `build/voice-budget-audit/snapshot/`, including tracked and untracked nonignored files. `status.txt` and `dirty.patch` record the starting checkout. Existing generated headers were copied read-only from the shared build into the isolated snapshot; subsequent preset probes, compilation and generated output all stayed inside the audit directory. Capture was a sequential filesystem copy, not an atomic repository transaction. It already contained in-progress sustain fields and engine gate changes; it is **not** the earlier pre-sustain .67 image, and is not a claim about the parent's later final sustain code.

Evidence and scripts reside under `build/voice-budget-audit/`:

- `target_audit.py`: baseline and `NVOICE=12/16` target links, with `NPOLY=8` unchanged.
- `prepare_candidates.py`, `candidate_audit.py`: separate shared-12 and weighted-12 experiments.
- `weighted12.patch`, `shared12.patch`, `fm6_8.patch`: reviewable diffs against the captured sources; apply only after reconciling with final sustain work.
- `host_audit.py`, `additional_audit.py`, `bench_audit.py`, `fm6_audit.py`, `fm6_bench.py`, `sustain_probe.py`, `sizes_audit.py`: tests and measurement commands.
- `manifest.json`: SHA-256 inventory of the baseline snapshot's firmware, tools, tests and copied generated headers at measurement time, including added audit probes. It is not a timestamped hash inventory of every original checkout file.
- Candidate directories retain their logs, ELF, disassembly, raw linked app image and isolated host binaries. These raw images are measurement artifacts, not installable packages.

Target compiler: installed pinned `/home/shaun/.cache/sloop-host-tools/jieli-linux-toolchains-20250324.1`, WSL Ubuntu; normal `build.build_app()` flags, boot tables/zones/presets enabled, SLICE disabled. Target links did not invoke packaging. Shared/weighted/FM6 candidates also ran `build.check()`: no errors, RAM-resident flash code has no calls, CPU1 loop remains in RAM, scratch bounds valid. Host compiler: installed Zig 0.13.0 `cc`, `-O2 -fwrapv -fno-strict-aliasing`, pthreads. No dependency installation was needed.

## Target image and RAM measurements

All values are actual isolated pi32v2 links, in bytes. Flash capacity is 584,336 B. Main RAM consumption includes linker alignment through `_bss_end`.

| Candidate | Flash | Flash free | Main RAM | Low RAM | Pool |
|---|---:|---:|---:|---:|---:|
| Captured sustain baseline | 583,432 | 904 | 89,472 | 28,928 | 331,852 |
| `NVOICE=12`, `NPOLY=8` | 583,544 | 792 | 90,816 | 28,928 | 331,852 |
| `NVOICE=16`, `NPOLY=8` | 583,592 | 744 | 92,160 | 28,928 | 331,852 |
| Shared admission 12, slots/caps unchanged | 583,432 | 904 | 89,472 | 28,928 | 331,852 |
| Weighted admission 24 units, slots/caps unchanged | 583,400 | 936 | 89,472 | 28,928 | 331,852 |
| FM6 cap 8, shared budget 8 | 583,432 | 904 | 89,472 | 28,928 | 331,852 |

Compared with the supplied pre-sustain .67 reference, the captured baseline is +468 B flash and +16 B main RAM. The weighted candidate's -32 B is an observed compiler/link result, not a promised future saving.

Region headroom at baseline: main 8,832 B of 98,304; low RAM 568 B of the 29,496 B remaining after RAM code; pool 12,212 B of 344,064. `build.check()` requires at least 8,192 B pool spare, leaving only **4,020 B** for pool growth within that policy. These regions cannot be treated as one fungible free-memory total. Core stacks and guard bands must remain intact.

The target ABI probe (`target_sizes.txt`, object symbol sizes, not host `sizeof`) measured:

| Object | Target bytes |
|---|---:|
| `voice_t` | 84 |
| `track_t` | 2,272 |
| `eng_arena_t`, per synth part | 8,592 |
| `fm6_note_t` | 296 |
| FM6 part, six notes plus two buses | 2,032 |
| GRAIN part | 7,020 |
| DRAWBAR part | 832 |
| PHYS small / big slot | 2,664 / 4,296 |
| `project_t` | 3,816 |

Older comments describing FM6 notes as 244 bytes are stale. Some host sidecars have different sizes because of host pointers; for example GRAIN is 7,120 B on the host. Use target measurements for RAM decisions.

There are four `track_t` objects, including the drum track, so each additional physical voice slot costs `4 * 84 = 336 B` main RAM. DRAWBAR's `v[NVOICE]` grows too, but remains under the PHYS-dominated arena for the measured 12/16 cases. The union, not the sum of its engine members, determines pool consumption.

## What the existing limits actually mean

`voice.c` scans eight slots per synth part, but admits only eight non-fading voices across the three parts. Six drum voices and the metronome are separate. Stage-4 stolen voices still render during their fade but do not count toward admission. Therefore eight admitted voices never meant eight render invocations in the worst transient.

`trk_nvoice()` clamps each engine to `NPOLY=8`, with these exceptions:

| Engine | Current per-part cap | State/cost constraint |
|---|---:|---|
| ANALOG, DIGITAL, PHASE, LOFI, SAMPLE, TRIO, WHEEL, NOISE | 8 | Generic voice state; WHEEL additionally owns per-slot sidecars in the arena |
| VOICE / FORMANT | 4 | CPU cap; shared random state constrains dual-core scheduling |
| FM6 | 6 | `note[FM6_POLY]`, operator envelopes and two per-part buses |
| GRAIN | 3 | Twelve shared grains, reverse windows, seek tables, per-part decode-load limit |
| PHYS MODAL / STRING / MEMB | 3 | Three small packed slots |
| PHYS SYMP | 2 | Two big packed slots |

Merely increasing `NVOICE` leaves the per-part ceiling at eight. Worse, `mono_play()` uses `NVOICE` to scale detune width and select velocity compensation. With `NVOICE=12`, eight-voice unison uses width 11 instead of 7 and switches from 36% velocity to half velocity. This is a sound change even before adding notes. Slot count, shared admission, poly cap and unison reference width/gain must be independent concepts before larger per-track counts are considered.

FM6 eight-note state would occupy `8 * 296 + 256 = 2,624 B`, still below the existing 8,592 B arena. The measured cap change therefore costs no additional static RAM. Raising PHYS is different: four small slots require 10,656 B per part, growing the pool by 6,192 B and violating the existing 8 KiB reserve. Three SYMP slots require 12,888 B per part, growing the pool by 12,888 B and exceeding the physical pool region by 676 B. Those are not safe scalar cap changes.

VOICE six could fit generic state, but would increase that part's worst-case rendering work by 50%; it was inspected, not linked or qualified here. Raising GRAIN's voice cap without changing its twelve-grain pool redistributes grains per note and can change timbre/density; increasing grains also requires adjusting normalization, reverse windows, seek/decode bounds and arena sizing. Neither is a content-preserving free increase. SAMPLE decoding is already IMA ADPCM and can advance up to eight source-rate multiples per output sample; high transposition is materially more expensive. No runtime sample recompression is proposed.

SLICE is compiled out in this configuration. Its `slc_rbuf[NPART][NVOICE][SLC_RB]` is a separate capacity trap for any future enabled configuration; the reported RAM figures do not qualify that build.

## Weighted prototype and patches

The useful prototype leaves `NVOICE=NPOLY=8`. It assigns two units to LOFI/WHEEL and three to every other engine, with a shared allowance of 24. This admits twelve cheap, eight normal, or intermediate mixed populations. All existing engine caps remain in effect. WHEEL and LOFI were selected for a conservative experiment from host profiles, not proven to be cheap for every target patch.

`weighted12.patch` adds `voice_cost()`, `voice_load()` and `voice_fits()`, then replaces the admission checks in both `voice_room()` and `voice_alloc()`. It repeatedly selects victims until the incoming weight fits; killing a single cheap voice is insufficient when replacing it with a heavier one. Reusing the requesting part's own victim retains the existing in-place restart behavior. Soft unison allocation still protects the part's own voices; hard admission still uses released voices, extra unison voices, then eligible held voices. CPU overload shedding remains unchanged.

This prototype recomputes load by scanning the existing 24 slots; it adds no mutable counter shared across cores. Keep admission on core 0 before jobs are dispatched. Any later incremental counter must cover same-note reuse, own-part steals, stage-4 transitions, engine-owned `done()`, one-shot endings, panic, reset and engine/preset fades; a stale counter is more dangerous than this small bounded scan.

Important qualification boundaries:

- It is a fixed engine-class weighted policy, not measured per-preset adaptive scheduling.
- Weights omit fades from admission, just as the current count does; actual rendering can briefly exceed the admitted cost. They also omit FX, drums, decoding pitch, USB interrupts and UI/flash work.
- The no-victim hard-allocation path relies on the current protected-lead limit and positive weights; current protected leads alone cannot consume 24 units. Revisit this assumption if parts, priorities or weights change.
- Per-part cap changes and weight changes must be checked together. This audit did not test a combined FM6-8 + weighted image.

`shared12.patch` is a two-comparison control experiment. It proves a larger shared budget needs no larger physical arrays, but admits twelve heavy voices as well. It is not the recommended release policy. `fm6_8.patch` changes only `FM6_POLY`, so allocation and sidecar dimensions stay tied to the same constant.

## Test evidence and limits

| Experiment | Results |
|---|---|
| Baseline | 200 golden renders, zero changed/gone, zero health/routing failures, zero crashes; random-use stress 40,000 frames; FM6 tests; upstream project import/apply/save identity; per-note ties/recording; sequencing ownership; harmony owner unit test; arena transitions; overload victim priority/fades all passed |
| Shared 12 | All 200 golden hashes and health checks unchanged; custom allocation/shedding probe, sequencing ownership, arena and split checks passed. Existing regression exits 1 on two eight-budget assumptions: budget count reaches 12; steal scenario generates only one take instead of its expected multiple takes. These failures were retained, not hidden by rewriting goldens/tests |
| Weighted 12 | All 200 goldens unchanged, zero health/routing failures/crashes; allocation/shedding and sustain-HOLD probes, sequencing ownership, arena and split checks passed |
| FM6 8 | All 200 goldens unchanged, zero health/routing failures/crashes; FM6 patch/bank/envelope/cap tests and arena transitions passed |
| Slot-only 12/16 | Actual target links and static instruction checks; no claim of host/audio qualification for these rejected designs |

Baseline, shared and weighted split tests each ran CORES=1, CORES=2, CORES=2 with CHAOS=7 and CORES=2 with injected timeout at job 700. Each split test checked seven scenes × eleven unit splits, comparing against its own unsplit audio. Baseline CPU1 transport tests also exercised those modes. This is host thread/deferred/fallback identity evidence, not hardware concurrency timing or a ThreadSanitizer race proof.

The weighted probe ran 10,000 mixed allocation/release blocks per scenario, checked every output sample's bounds, injected `shed_voice()` calls, then verified all voices freed after cleanup. Cheap-only peak was 12 voices / 24 units; normal-only 8 / 24; mixed cheap + FM6 10 / 24; VOICE + FM6 + PHYS 8 / 24. The same four scenarios passed with sustain enabled in HOLD mode. This tests the captured sustain behavior, not later sustain revisions or every pedal routing path. Existing `seq2_test` independently passed releasing-first shedding, protected bass/mono lead and stage-4 fade checks.

USB audio descriptor/ring tests passed with `T_CDC=0,1,2`, `HALF_FRAMES=256`; their scheduled render durations are simulated, not DSP deadline measurements. Initial ad-hoc invocations omitted required compile flags (`ARENA_STATS`, harmony implementation linkage, UAC defines); they were corrected and rerun successfully. AddressSanitizer instrumentation could not link with the installed Zig runtime (`__asan_*` symbols unavailable); no ASan pass is claimed. ThreadSanitizer was not run.

The repository's target instruction estimator passed for baseline, slot-only, shared, weighted and FM6 candidates. It counts static loop instructions with heuristic nesting/divide weights. It neither measures AC7911 cycles nor accounts for a higher dynamic voice population; unchanged static render costs do not establish unchanged deadline usage.

Matched host workloads, unsplit (`CPU1_UNITS=0`), eight notes requested per participating part, factory preset 0, best of three runs of 3.2 million output frames:

| Workload | Baseline busy / ns per sample | Shared 12 | Weighted 12 |
|---|---:|---:|---:|
| Three LOFI parts | 8 / 49.692 | 12 / 60.565 | 12 / 64.175 |
| Three WHEEL parts | 8 / 55.374 | 12 / 69.064 | 12 / 73.248 |
| Three ANALOG parts | 8 / 88.108 | 12 / 118.138 | 8 / 93.498 |

Separate sustained FM6 BRASS SECT test: baseline six active voices, 90.176 ns/sample; cap-eight candidate eight active voices, 112.200 ns/sample (+24.4% whole-mix host time). The original TINE EP benchmark decayed to zero active voices; its idle results in `bench.log` are invalid for FM6 capacity comparison and are superseded by `fm6_bench.log`. Sequential runs still have host noise; these are screening measurements, not target speed ratios or deadline guarantees. No CPU baselines or golden files were updated.

## Scheduler, ownership and format audit

The runtime `track_t` is not serialized wholesale. `project.c` captures fixed project fields explicitly. The target project remains 3,816 B, below the 3,840 B storage payload. Step and drum-step structures remain ten bytes; stored steps, direct sequencer chords and recording ownership are limited to four notes. Expanding simultaneous tails does not expand stored chord width. Preserve that separation and upstream format IDs.

Other independent bounds that must not be blindly scaled with voice storage:

- Eight-entry mono last/high/low-priority stack uses its own array bound. Sixteen held arp input notes expand into 64-note runtime lists; these are input ownership capacities, not synth voice indices.
- Four-note keyboard chord, latch, `seq_notes`, recorded `rh_note/rh_off` arrays and gate/continuation bitfields are tied to the saved four-note schema. Existing note masks span 128 MIDI pitches; track/unit masks identify tracks, not voices.
- Strum queue `STQ=16` falls back to immediate play when full; four roll slots are independent. Admission steals must never fabricate MIDI note-offs or remove logical input owners.
- USB/TRS input queue `MQ=64`, SysEx queue 64, MIDI output queue 512 and UI step-entry queue 32 are event capacities. More audible tails do not automatically demand resizing, but dense generated-note/pedal bursts must be tested for loss and balanced note-off delivery.
- GRAIN owners are byte-sized voice index + 1; FM6 sidecars use the engine cap; DRAWBAR indexes by slot; PHYS uses model-dependent packed strides. Preserve all bounds and engine arena ownership when voices end or engines change.
- `project.c:audio_quiet()` scans every active synth voice before autosave. Longer sustain/HOLD can intentionally defer idle autosave; do not redefine a cheap or silent logical owner as permission to erase flash while audio is active.

No voice-index bitmask bottleneck was found in production allocation for the current eight slots; test/UI diagnostic masks do exist. That finding does not license arbitrary counts above 32 or counts beyond byte-index ranges. UI scanning, main RAM growth and all engine sidecars must be reevaluated for any future larger slot design.

## CPU1 and deadline constraints

At 44.1 kHz, 256-frame audio halves allow about **5,805 microseconds**, rendered in eight 32-frame blocks. Two consecutive halves above 85% wall time request shedding on the following half; only one voice is shed per half. A single burst does not shed. Stage-4 fades take two blocks, about 1.45 ms, and still cost DSP work. This is reactive recovery, not a guarantee against the first missed DMA deadline.

Default CPU1 work is the last synth part plus drums, using a mask of whole units. A large chord on track 1 stays on core 0; a large chord on track 3 plus drums may overload CPU1. Parts remain on core 0 when their S&H LFO wraps, PHYS rendering would draw shared RNG, or multiple FORMANT parts need ordered shared random state. Global weight alone cannot establish either core's worst-case budget. Dynamic whole-track balancing could help, but must preserve these exclusions, RNG ordering and per-part ownership; splitting one track's voices needs a new race/state audit.

CPU1 timeout is eight milliseconds, already longer than an audio-half deadline. A timeout holds CPU1 and falls back to core 0. Passing timeout identity tests does not mean that recovery half meets deadline. Audio load diagnostics exclude nested TIMER5 time, while overload detection uses wall time including nested USB work. Judge lateness and full wall time, not just smoothed `song.cpu_q8` or `audio_max_us`.

## Cheaper release and sustain tails

Released/sustained voices currently run the same oscillators, filters, model loops or decoder as held voices until completion. Envelope and tail-gain work is control-rate and small. Existing silent-sustain skipping already avoids rendering certain zero-envelope held voices; it does not make arbitrary audible releases cheap. FM6 uses operator envelopes for completion, SAMPLE must advance decoder/position, GRAIN maintains its grain pool, and PHYS models retain resonator state.

Safe priorities are better admission and earlier reuse of logically released voices, plus measured engine-specific exact optimizations. Cache common control coefficients if profiling justifies it. A final block with both endpoint amplitudes zero and a permanently completed voice may be skipped only after verifying engine completion and cleanup; an amplitude-zero held or resumable sample voice cannot generally stop state advancement.

Reducing tail sample rate, dropping operators/modes/grains, replacing an audible tail with a generic fade, or stopping at a new audibility threshold changes sound. Such approaches are not included in the content-preserving candidates. The weighted patch keeps releasing/sustained voices eligible under the existing victim policy and does not discount their weight merely because a key was released.

## Hardware acceptance plan before adoption

Qualify each candidate separately against an otherwise identical final-sustain baseline, using the pinned compiler and unchanged samples/presets. No hardware deadline measurements were available in this audit.

1. Capture full audio-half wall time (including nested TIMER5), DMA `late` deltas, overload/shed counts, CPU1 job/wait times, timeout count and both stack low-water marks. Existing smoothed load values need temporary per-half maxima or a GPIO/logic-analyzer trace to expose bursts. Confirm RAM guards, boot arena zeroing and audio startup before musical stress.
2. Test both cores enabled, disabled, and forced failure/fallback. Concentrate twelve cheap voices on tracks 1+2, then 2+3 and 1+3; rotate an eight-note chord through each track. Add all six drum voices, metronome, track inserts/filters, sends, worst reverb/delay, texture and active punch FX. Repeat at maximum supported USB audio rate with USB MIDI, TRS MIDI and UI drawing active.
3. Sweep every LOFI/WHEEL factory preset and legal expensive macro combination, eight-key unison plus other tracks, fast reattack/release churn, maximum sustain release and HOLD, pedal release/repress, long release stealing, and same-note retrigger. For FM6 test eight simultaneous six-operator voices across all 32 algorithms, high feedback and worst macros, then six/eight voice A/B; include three FM6 tracks under the unchanged shared eight-voice policy.
4. Force CPU1 exclusions repeatedly: LFO wraps on all tracks, three FORMANT tracks, PHYS MODEL flips under notes, engine/preset transitions involving FM6/GRAIN/PHYS/WHEEL. Add high-transposition SAMPLE playback and reverse/high-density GRAIN on companion tracks. These stress the core-0 path and flash fetch contention even when cheap admission is the feature under test.
5. Run dense four-note sequencer chords with overlapping ties, arp expansion, strums/roll queue saturation, swing, live recording, MIDI routing changes, sustain, mute/solo, STOP/panic and project/scene changes. Verify balanced outgoing MIDI and complete cleanup, not just silence. Confirm upstream project round trips and all instrument bytes remain identical.
6. Require zero DMA-late increments/timeouts/guard failures in supported steady-state workloads and maximum full-half wall time below the half deadline, preferably below 85% (about 4,934 microseconds) to retain interrupt margin. Deliberately overload separately: validate release-first shedding, protected bass/lead, bounded recovery and no hanging owner. If single-core fallback cannot meet these conditions with the larger budget, lower admission on fallback or retain the original budget there; the present prototype does not implement that policy.
7. Run at least a 30-minute worst-case sustain/FX/USB soak plus a long randomized engine-change session. Record raw maxima and workload details. Re-run golden/ownership/two-core tests on the exact final binary sources after any weight, cap, scheduler or sustain change. A clean average CPU display is insufficient evidence.

Adopt weighted admission or FM6 eight only after this evidence establishes headroom. The blanket shared-12 control and global slot increases have no such recommendation.

## Production candidate integration

User-approved weighted admission and FM6 eight are now integrated with the final Q15.9 sustain foundation. The actual combined image is **583,776 / 584,336 B** (560 B free); static main/low/pool RAM 89,616 / 28,928 / 331,852 B, unchanged versus .68. Existing slots, unison gain, other engine caps and overload shedding remain. A new regression exposed fading same-note reuse bypassing admission; `voice_start` now reacquires room for active stage-4 slots before restarting them. `tests/voice_budget_test.c` proves twelve cheap voices, eight FM6 voices, mixed sustained-tail eviction and fade-retrigger readmission, including sanitizer/two-core runs. Existing storage/audio/ownership tests accompany it. This makes the policy available for device evaluation; host checks do not establish AC7911 deadlines or per-core worst-case margin. The hardware matrix above remains pending, and weights may need reducing after measurement.

## Adjustable-budget experiment

The subsequent hardware-evaluation build offers HOME → AUDIO → VOICE BUDGET: default 8 / 12 (24 units) or 10 / 15 (30 units), runtime-only. Per-track engine caps remain. Per-pitch pending recovery covers both cross-track fades and in-place drone steals, without extra MIDI/recording events or stealing to recover. Final target 584,280 B (56 B free), main static 90,336 B (7,968 B free). The native ordered clamp from the prior audit funds part of this integration; no sample compression or new voice slots. See [controls, tests and qualification](VOICE-BUDGET-EXPERIMENT.md).

The wider selector now extends admission to 48 units (16 normal or 24 cheap voices shared). It adds 20 B flash and no RAM versus the initial adjustable experiment: 584,300 B, 36 B free. Per-track caps remain unchanged. Every admission boundary and menu wrap/clamp is covered by the maintained recovery test.
