# Ambient/drone headroom audit synthesis

## Baseline and conclusion

All eight specialist reports reproduce the same MerthSloop 2.8.0.58 target:
583,388 / 584,336 application bytes, 948 B flash free; main static RAM
86,176 / 98,304 B (12,128 B free); low RAM 28,928 / 29,496 B (568 B free).
Pool occupancy is 332,876 / 344,064 B. Retaining its 8,192 B reserve leaves
2,996 B above that reserve. Static margins do not establish stack high-water.

The audits themselves changed no production firmware. The first integration now
implements the RAM-state combination, portable C clamp and focused boot packing:
**1,392 B flash recovered**, leaving **2,340 B**. Main RAM uses **86,320 B**,
low RAM **28,928 B**, pool **331,852 B**. These measured combined results supersede
the planning arithmetic below; the individual audit measurements remain frozen.
The results establish several useful sub-KiB wins and two larger alternatives,
not a large unclaimed dead-code pool. Prior integrated savings are excluded.

## Ranked measured candidates

Each row is a complete independent target link against the baseline. Rows are
not additive guarantees; only combinations named within an individual report
were actually linked. Positive RAM cost consumes headroom.

| Candidate | Flash recovered | Main RAM change | Low/pool change | Assessment |
|---|---:|---:|---|---|
| RAM-state combined: insert union, sample decode scratch, FFT scratch | 48 B | -880 B | pool -1,024 B | First batch; state/lifetime proofs and focused equivalence tests |
| Ordered constant-bound clamp specialization, portable C | 544 B | 0 static | 0 | First batch; retain original dynamic/inverted-bound fallback; measure stack/deadlines |
| Focused startup packing: two numeric tables, zero sentinel, preset field bits | 880 B | +1,008 B | 0 | First batch; playback remains direct; integrate generators and host startup |
| Larger startup packing: focused plus zones and six-bit names | 1,728 B | +4,000 B | 0 | Alternative to focused, not an additional 1,728 B; add unsupported-name fallback |
| Shared analog renderer/filter calculation | 1,856 B | 0 static | 0 | Conditional; bit-exact host evidence, intentional LP specialization removed |
| Inline pi32v2 clamp specialization | 1,056 B | 0 static | 0 | Alternative to C; only 512 B extra over C; target execution/hardware gate |
| Cold initialization of dust/live-mod state | 288 B | 0 extent | 0 | Smaller follow-up; preserve native/WASM/test cold-start initialization |
| Dead TRACKS graph forwarding | 16 B | 0 | 0 | Optional small cleanup |
| Shared bounded chord-scale walker | 32 B | 0 | 0 | Optional small cleanup |
| Renderer tile flag widths | 0 | -112 B | 0 | Optional RAM housekeeping |

The RAM combined result is measured as a group. Its insert union saves 368 B
main RAM and 48 B flash; upload scratch saves 512 B main RAM; FFT saves 1,024 B
pool. The independent RAM-buffer report recovers the same FFT allocation using
a different alias. Choose one implementation; never count or apply both.
Prefer the RAM-state version's existing second FFT scratch vector initially,
which preserves waveform snapshots and belongs to its measured combination.

Preset relocation frees 6,156 B low RAM while consuming 6,160 B main RAM.
It saves no flash and increases total RAM by four bytes. Keep it separate and
move only the banks needed after choosing the ambient resident-state budget.
No subset relocation saving has been target-linked yet. Normal preset pointer
access does not require the low region; this is placement flexibility, not new
memory. Low data space is not automatically executable RAM.

## Suggested sequence and budget envelopes

1. Integrate the RAM-state combination and portable C clamp with regression
   oracles, real UI redraws, sample upload/backup failures and dual-core tests.
2. Integrate focused startup packing through normal generators/startup, preserving
   raw host builds and all restored bytes. Add boot-enabled native/WASM coverage.
3. Rebuild the actual combination and measure all regions. Run the complete audio
   golden suite, source provenance/preview parity and deployment checks.
4. Consider the 288 B cold-init follow-up and 48 B total tiny UI/chord cleanups
   only after preserving every bootstrap's initialization contract.
5. Evaluate shared analog rendering separately under worst-case target playback,
   both-core stack high-water and audio deadlines. Extend static budget checks to
   follow the moved analog_body loop instead of mistaking a tiny dispatcher for
   a faster renderer. Consider ASM only if its additional 512 B is needed.

Arithmetic planning only: RAM combination + C clamp + focused boot packing
suggests 1,472 B recovered, or about 2,420 B flash free. Main margin would be
about 12,000 B, low margin 568 B, and pool margin above reserve 4,020 B.
With the cold-init and tiny cleanups, the arithmetic flash margin is 2,756 B.
These are not measured combined links; global layout changes can alter them.

A broader envelope using the larger packing option, shared analog renderer,
C clamp, RAM combination, cold initialization and tiny cleanups is 5,460 B
flash free arithmetically; choosing ASM instead suggests 5,972 B. Treat this as
roughly 5–6 KiB of conditional potential, not a promised build. Without preset
relocation the larger packing plus RAM reductions leaves roughly 9,008 B main
margin; adding complete relocation would reduce that to about 2,848 B. Avoid
spending both budgets before sizing ambient state. None of these envelopes
includes a new feature, boot-time number or measured hardware speed claim.

## What not to integrate

- Runtime sample decompression: intentionally reverted; remains excluded.
- Whole factory-sample decompression at boot: 303,054 B resident destination
  cannot fit the proven free regions. Better compressed payload ratios do not
  solve destination capacity.
- Blanket packing of eight more numeric tables: grows flash 336 B while using
  1,840 B main RAM. Compressibility is not net linked savings.
- Post-render audio scratch overlay: saves 768 B main RAM but costs 1,168 B
  flash in its measured implementation; no fit at baseline. It is an explicit
  RAM-for-flash trade, not an optimization to take automatically.
- Blanket helper outlining, aggressive section alignment, GC or function-body
  deduplication: fresh target evidence is zero/trivial or regresses size. Only
  seven loaded alignment bytes and 32 gross exact-duplicate bytes were found.
- Unrestricted min/max clamp replacements: change inverted-bound behavior.
- General above-unity mulq16 replacement: preserve the sampler gain-overflow
  correction and proven coefficient domains. Signed-wide Q16 saves only 48 B
  independently and does not justify a broad arithmetic rewrite.

## Performance, compatibility and test limitations

Boot packing uses the existing decoder with no new render-time decoding. Restored
RAM addresses can still affect target code/data access; no exact boot-time or
hardware cycle improvement is proven. The shared analog candidate is sample/state
exact across the host oracle and all 200 goldens, but its static weighted LP loop
estimate rises about 4.5%; that is not a measured cycle or CPU percentage change.
C/ASM clamps alter register allocation; mixer local frame rises from 188 to 204 B.
The actual new ASM has not run on target hardware/emulator, only its host fallback
and equivalent emitted leaf instructions have been checked.

Preserve factory IDs/content, independent ties, latch/modifier semantics, live
editing, upstream project layout, MIDI protocols and current audio quality. Carry
relevant private differential/oracle tests into maintained regression coverage,
not private paths alone. A stale original ui_refresh expectation still refers to
FLUTTR; both baseline and candidate fail it, while a private current-label version
passes. Fix that production expectation honestly during integration rather than
claiming the original suite passed or weakening its redraw assertions.

## Reports

- [UI](AMBIENT-HEADROOM-UI.md)
- [Sequencing and protocols](AMBIENT-HEADROOM-SEQUENCING.md)
- [DSP](AMBIENT-HEADROOM-DSP.md)
- [Link/layout](AMBIENT-HEADROOM-LINK.md)
- [C and assembly](AMBIENT-HEADROOM-ASM.md)
- [Boot packing](AMBIENT-HEADROOM-BOOT-PACKING.md)
- [RAM state](AMBIENT-HEADROOM-RAM-STATE.md)
- [RAM buffers](AMBIENT-HEADROOM-RAM-BUFFERS.md)

## Second integration: metadata packing and small source sharing

The current build integrates the previously deferred zone metadata and six-bit
factory-name packing. The generator extracts target-compiled zone bytes from
the existing raw ELF probe, checks that they contain no relocations, and emits
startup data and a byte oracle. It reuses the existing boot decoder; restored
arrays and string pointers are accessed directly during playback. Factory names
outside the compact uppercase/punctuation alphabet fall back to ordinary LZ4
without changing their spelling. Sample payloads and standard ADPCM playback
are unchanged; no new render-time decompression is introduced.

The duplicate TRACKS dispatch in draw_graph is removed: ui_draw already handles
that screen. The two bounded chord scale-walking loops share one helper with
an exhaustive 786,432-case reference check. Independent REV HOLD was removed
at the user's request; upstream's FREEZE punch-in remains intact.

| Actual target link | App B | Flash free B | Main static RAM B |
| --- | ---: | ---: | ---: |
| Prior .63, including native REV HOLD | 584,000 | 336 | 86,432 |
| Metadata/name packing, before HOLD removal and tiny cleanups | 583,156 | 1,180 | 89,408 |
| Retained .64: packing, HOLD removal, shared chord walk and dead dispatch cleanup | **582,804** | **1,532** | **89,408** |
| Same .64 candidate with experimental cold-state conversion | 582,868 | 1,468 | 89,392 |

Net recovery is **1,196 B flash** versus .63. Restored metadata/names consume
**2,976 B additional main RAM** including alignment, leaving **8,896 B**.
Low RAM remains **28,928 / 29,496 B** (568 B free); pool remains
**331,852 / 344,064 B**, 4,020 B above its unchanged 8,192 B reserve.
Individual source/data savings must not be summed instead of these combined
links. No audio buffer, voice, instrument, preset ID, file or protocol format
was removed or changed, apart from the explicitly removed independent HOLD UI.

The old cold-state result does not carry forward: its actual current A/B costs
**64 B more flash**, recovering only 16 B main RAM. It is rejected, and original
declaration defaults are retained in every bootstrap. No experimental cold-state
flag/helper remains. Shared analog rendering and ASM remain deferred because
they change hot-path code and need target performance evidence. Preset relocation
buys low RAM rather than flash; the UI bitfield candidate primarily buys RAM.
Neither belongs in this flash-only batch.

Validation: byte-identical restored 1,428-byte zone table, 96-byte startup-state
oracle and all 171 complete presets/names; name alphabet/fallback and bit-boundary
tests; **200 unchanged audio goldens in each raw and boot-restored build**;
shared chord-walk reference, UI redraw/fuzz, legacy scale/latch and progression,
drone ownership, original punch-FREEZE, delayed two-core splits, native/WASM
parity, linker placement and hot-loop budget checks. Original decoder bounds,
repeat restore and guarded preset scratch tests remain. Hardware cold/recovery
boot latency, stack and deadlines have not been measured for this batch.

## Third integration: build-time compressor search

The existing metadata decoder is retained; improved Python-side match selection
recovers **768 B** on a same-source/version A/B. The .66 HOME lane visibility
fix leaves the same final size: **582,212 B**, **2,124 B flash free**, with main,
low and pool RAM unchanged from .65. Raw/restored byte oracles, all 171 presets,
200 audio goldens, UI redraw/foreground checks and target budgets pass. See
[implementation and evidence](OPTIMIZATION.md#broader-build-time-lz4-match-search).
