# Eleven physical chord modifiers

The .49 image is **584,336/584,336 bytes**, **0 free**, eight bytes over .48.
Main RAM remains **89,128/98,304**, low RAM **28,280/29,496**, audio pool
**332,876/344,064**. The eleven-bit latch set grows logically by three bytes across
three synths, absorbed by existing alignment. No audio voices/buffers are added.

A compact physical-key index table replaces repeated pitch-class modifiers. One
shared extension insertion path replaces the old repeated seventh/ninth logic;
existing harmony_shape handles both inversion directions and open voicing.
Automatic voice-leading history excludes explicit register gestures. Common
capacity/lock messages are reused rather than adding synonymous strings.

The new work runs at note/modifier events, not in sample playback loops. Target
placement/budgets, 182 unchanged ordinary audio renders, chord/scale/arp/voicing
regressions, UI fuzz and normal/UBSan exhaustive modifier tests pass. The latter
checks all 3,932,160 shape/scale/register/mask combinations, plus real physical and
latched direct/arp ownership. Hardware feel remains user validation.

# Conditional SEL guide and adjustable scale LEDs

The .48 image is **584,328/584,336 bytes**, **8 free**. Restoring the normal scale
sketch and adding saved guide brightness fit by sharing byte menu toggles and
note-move completion paths, keeping cold edit handlers out of line, rotating the
12-bit scale mask once per drawing rather than taking a remainder per key, and
precomputing adjustable LED timer ticks outside the scan interrupt.

Linked RAM remains main **89,128/98,304**, low **28,280/29,496**, pool
**332,876/344,064**. There are no new audio buffers or voices. The maximum dim
pulse increases from 4 us to 32 us only while the persistent guide owns the playing
keyboard. Hardware brightness/scan-load validation is still needed; host tests do
not measure electrical LEDs or interrupt latency. Target checks and focused UI,
scale settings, note-move, transpose and starter regressions pass.

# Compact chord-screen guide

The .47 image is **584,336/584,336 bytes**, **0 free**, adding **8 image bytes**
over .46. The SEL modifier legend replaces the scale sketch. It uses only static
strings and the existing canvas; linked RAM, audio processing and input behavior
are unchanged. Target placement and budget checks pass.

# Live editing and progression control flow

The .46 image is **584,328/584,336 bytes**, **8 free**, adding **168 image bytes**
over .45. Linked RAM is unchanged: main **89,128/98,304**, low **28,280/29,496**,
pool **332,876/344,064**. Live note edits reuse `seq_skip` to suppress stale
current-step ratchets; normal step processing reconciles held notes. No new state,
audio buffers, scheduler or persistence layout. The cold progression takeover helper
is explicitly kept out of line. Target placement and budget checks pass.

# Radio/Dub/Dark response tuning

The .45 image is **584,160/584,336 bytes**, **176 free**, adding **384 image bytes**
over .44 with unchanged linked RAM: main **89,128/98,304**, low **28,280/29,496**,
pool **332,876/344,064**. No new audio ring, voices or scheduler. Radio gains an
explicit early band mix and receiver hiss; Dub reuses the mutually exclusive
CRUSH held state for a bass-cut filter and shares a small concave amount mapping
with Radio. The tiny delay mode field fits existing linker alignment. Dark changes
coefficient/return tuning. Target RAM/flash placement and hot DSP checks pass.

# Radio, Dark, Dub and direct progression chords

The .44 image is **583,776/584,336 bytes**, **560 free**. Main static RAM is
**89,128/98,304** (9,176 free), low RAM **28,280/29,496**, pool **332,876/344,064**.
The three effects plus direct chord output add **608 image bytes / 16 main static
RAM bytes** over .43. Radio adds two logical high-pass states (8 bytes, linked
alignment yields 16). Dub reuses the delay saturator/filter; Dark reuses FDN
coefficient tables and damped line states. Direct chords reuse the generated-note
list/emitter and regular held recorder. No extra audio rings, voices, sample
compression, scheduler or format expansion. Target budgets/placement/inlining,
normal audio goldens, effect responses, storage/UI and sanitizer checks pass.

# Integrated progression performance build

The .43 image is **583,168/584,336 bytes**, **1,168 free**. Main static RAM is
**89,112/98,304** (9,192 free); low RAM **28,280/29,496**; pool **332,876/344,064**.
Compared with final .42, the complete NOW-only browser/pad/recording/lifecycle
slice adds **2,516 image bytes / 16 main static RAM bytes**. The finished UI and
ownership checks exceed the original 2 KiB planning target by 468 bytes; this
measured image still fits with 1,168 bytes of margin. No new audio buffer,
per-sample work, heap or scheduler was added. RAM placement, target DSP cost and
24 hot-helper inlining checks pass. The isolated 1,272-byte private kernel was
not a finished-feature measurement. See [performance](PROGRESSION-PERFORM-DESIGN.md).

# Selected-character LOFI punch build

The final .42 image is **580,652/584,336 bytes**, **3,684 free**.
LOFI punch reuses the existing state/ring, retains the selected character and
uses Vinyl only when OFF. LOFI routing adds 96 bytes before a shared-sort cleanup recovers 48. The net
increment is 48 image bytes over the voicing-only
measurement below; main/low/pool RAM is unchanged.

# Native voicing build

MerthSloop 2.7.1.42: **580,604/584,336 image bytes**, **3,732 free**.
Main RAM **89,096/98,304**, low RAM **28,280/29,496**, pool **332,876/344,064**.
Native inversion/spread UI and optional project metadata add **544 flash bytes**
and **16 main RAM bytes** against the packaged .41 baseline. No new buffers,
voices or per-sample work. See [voicing](CHORD-VOICING-DESIGN.md).
Earlier measurements below retain their own baselines.

# Packaged cleanup build

MerthSloop 2.7.1.41: **580,060/584,336 application bytes**, **4,276 free**.
Main static RAM is **89,080/98,304**, low RAM **28,280/29,496**, pool
**332,876/344,064**. The four-byte difference from the frozen redundancy
comparison below comes with the new version/build, not a lost optimization.

# Working-source redundancy cleanup

An isolated target build reproduces the served 580,312-byte baseline, then
measures **580,056 bytes** after sharing panel/MIDI entry setup and HOME/parameter
card rendering: **256 bytes saved**, **4,280 free**, RAM unchanged. All 116 UI
captures are byte-identical; actual UI fuzz, entry/delay tests and target DSP
checks pass. The helpers are now included in the separate release above;
the earlier package figures below retain their original measured baselines.
See [redundancy audit](REDUNDANCY-AUDIT.md) and
[upstream candidates](UPSTREAM-OPTIMIZATION-CANDIDATES.md).

# Current adjustable-delay and preset-storage build

MerthSloop 2.7.1.40: **580,312 image bytes**, **4,024 free** in a
584,336-byte slot; main static RAM **89,080/98,304**, low RAM
**28,280/29,496**, pool **332,876/344,064**. DRIVE/TONE and BITS/RATE
controls are integrated; master LOFI starts at OFF.

The controlled factory-preset experiment saves **2,580 bytes net**, restoring
153 unchanged records into 5,508 bytes of spare low RAM once before audio starts.
Another **2,772 bytes of capacity** comes from trailing space inside the same
0x4000..0x93000 update region, with SDK payload-length checks. These are distinct
gains, not two code-size savings. No audio buffer, sample decoder or playback
access changes. Actual restored preset comparisons and 182 audio goldens pass;
target placement, inlining and DSP budgets pass. Hardware startup duration and
SPL boot of the expanded directory remain unverified. See the
[ranked architectural investigation](ARCHITECTURAL-HEADROOM.md).

Earlier checkpoints below retain their own baselines and capacities.

# Current delay build

MerthSloop 2.7.1.39: **580,984 image bytes**, **580 free**; main static RAM
**89,080/98,304**, low RAM **22,772/29,496**, pool **332,876/344,064**.
Adds compact delay characters, complete native controls/storage, the zoom hint
and LOFI OFF default. Compared with the prior production build, image grows
276 bytes and main static RAM grows 16. All existing audio ring sizes remain
unchanged. Settled DIGI skips coloration arithmetic; active TAPE/CRUSH have
additional per-sample work. Target DSP budgets pass; physical timing is unmeasured.

# Current piano-roll display build

MerthSloop 2.7.1.38: **580,708 image bytes**, **856 free**; main static RAM
**89,064/98,304**, low RAM **22,772/29,496**, pool **332,876/344,064**.
Includes the piano-roll grid and LOFI OFF. Sharing clamp only within the UI
reduces duplicated code without changing the inline DSP helpers. Final image
placement/no-call checks and target DSP budgets pass. Earlier snapshots below
retain their own measured sizes; do not add deltas from different snapshots.

# Current integrated editor build

MerthSloop 2.7.1.37: **581,140 image bytes**, **424 free**, main static RAM
**89,064/98,304**, low RAM **22,772/29,496**, pool **332,876/344,064**.
The complete piano editor adds 7,768 bytes in its controlled comparison.
Compact descriptors and startup-only numeric/FIR/FM6 table packing recover
4,448 bytes against the audit snapshot. No sample-loop decoder is introduced;
factory samples retain direct reads. Restored bytes and coefficients match their
raw oracles; 182 audio renders are unchanged. Target DSP budgets and image
placement checks pass. Startup duration/watchdog margin needs hardware validation.
See [space audit](PIANO-ROLL-SPACE-AUDIT.md), [editor measurement](PIANO-ROLL-SIZE.md)
and [compiler experiments](COMPILER-FLAGS-AUDIT.md). Toolchain/LTO experiments
remain separate from this build, which uses the existing compiler and flags.

The checkpoints below are historical measurements.

# Current effects build

MerthSloop 2.7.1.36: **580,304 image bytes**, **1,260 free**, main static RAM
**85,688/98,304**, low RAM **22,772/29,496**, pool **332,876/344,064**.
VINYL reuses its existing ring; stronger modulation and SPRING feedback each add
one small smoothed/control value. DOUBLE TIME removes the former octave shifter.
The reverb type table shares one FDN implementation; its build measured 1,096
bytes smaller than 2.7.1.34 despite the added types. The subsequent selective
upstream fixes add 192 image bytes and 32 main static RAM bytes. Existing
sample decoding, engines and boot presentation remain intact; no new audio buffer
is allocated. See [reverb expansion](REVERB-EXPANSION.md) and
[effects implementation](EFFECTS-IMPLEMENTATION.md) for integration and limits.
Figures below retain earlier checkpoints as historical evidence.

# Unreleased effects workbench

HALF transport, limited punch combinations and optional SPRING plus persistent VINYL build to 580,428
image bytes (1,136 free), with 85,624 bytes main static RAM. Audio pool remains
332,876/344,064; low RAM remains 22,772. Target cost/inlining checks pass;
physical audio timing is unmeasured. See [implementation and limits](EFFECTS-IMPLEMENTATION.md).
The following figures describe the preceding packaged 2.7.1.30 build.

# Current raw-sample build

MerthSloop 2.7.1.30 retains direct factory ADPCM reads. There is no factory sample
Huffman decoder, per-voice cache or block decompression in the render path.
Phone/computer playback into the FM-1 and phone access to sequence starters are
removed; upstream USB recording/MIDI and the native sequence browser remain.

The image is **575,160 bytes**, leaving **6,404 bytes of flash**. Main static RAM
is **81,592/98,304**, low RAM **22,772**, and audio pool **332,876/344,064**.

Twenty immutable generated lookup/font/drum-metadata tables (20,122 bytes) are
stored as 8,212 bytes plus decoder/descriptors. They expand once into ordinary
RAM during cold startup, before audio interrupts or core 1 start. Prediction and
LZ4 decoding are absent from playback. Output is compared byte-for-byte against
the original generated tables; normal and startup-expanded audio renders match.
Malformed compressed input is bounds-checked; boot enters ROM recovery on failure.
This uses the existing LZ4 block compressor and keeps generated assets reproducible.

The native UI refresh replaces the old vertical track renderer and ring dials.
Including the beat pulse, seven-style visualizer selection, consistent SONG/REC layouts and
section-save fixes, it saves **3,504 image bytes and 1,184 bytes of static RAM**
against the preceding raw-sample build. There is no framebuffer or pool growth.
Removing unused visualizers and animation state alone reclaimed 2,912 image
bytes and 48 static RAM bytes; durable-save handling adds 232 image bytes.
Card borders use two fills instead of five, preserving pixels with extra
background overdraw only on card updates. Physical UI timing remains unmeasured;
the user has reported the preceding layout looks good on hardware.

The seven retained styles are Oscilloscope, Spectrum, Spectrogram, VU Meters,
Note Trails, Groove and Song Journey. Their historical persisted IDs survive
compact menu numbering; removed IDs fall back to Oscilloscope. The historical
optimization evidence below describes earlier checkpoints, including code and
styles that have since been removed.

The engine-specific diagram prototype is withdrawn. Shared cards now serve
ordinary editing, SONG and record-ready/count-in views; the legacy bottom-dial
branch and duplicate large record header are removed. This consistency phase
reclaims 1,944 image bytes over the static-analysis checkpoint, with unchanged
RAM.

The screen-by-screen polish plus live octave-down replacement add 160 image bytes
over that shared-layout checkpoint, without static RAM or pool growth. Settings
navigation, credits, long parameter values and modulation guide extrema now fit
their native regions; see the [polish audit](UI-POLISH-AUDIT.md).

The static-analysis cleanup removes a right-channel scope ring and write-only
volatile animation flags left behind by the removed visualizers. Target savings:
32 image bytes and 1,040 main static RAM bytes. The audio ISR's static loop cost
falls from 268 to 256, with three loop call sites unchanged. This is a compiler
cost estimate, not measured hardware cycles. All seven visualizer captures are
byte-identical; all 182 audio goldens are unchanged. See the
[static-analysis audit](STATIC-ANALYSIS.md) for scope and deferred candidates.

# Integrated build measurements

MerthSloop 2.7.1.19 fits upstream dual-core/features and the fork workflows in
559,992 image bytes: 21,572 flash bytes free. Static RAM is 74,888/98,304,
low RAM 22,772 bytes and pool 333,308/344,064. Main-loop histories/undo use 8,800
bytes inside the explicitly zeroed RAM region, disjoint from core 1 code/stacks.

Factory IMA ADPCM remains byte-identical. A static Huffman tree packs independent
128-byte blocks; ROM offsets, fast decode table and per-decoder caches support
random seeks without a shared mutable cache. Per-grain caches prevent unrelated
grains from evicting each other's blocks. Uploaded samples bypass this layer and
retain upstream XIP layout and loop semantics. The generated header includes a
raw branch for verification (`SMP_ROM_PACKED=0`).

Raw factory bytes 303,054 → packed payload 259,930 plus offsets 7,104, fast table 512
and tree 1,020 = 268,566 bytes. Comparing complete builds with packing off/on gives
593,736 → 559,992 bytes: **33,744 bytes net flash saved**, including decoder code
and compiler layout changes. The raw build exceeds the unchanged flash limit by
12,172 bytes; it is only an analysis build and is never packaged for installation.

Every byte matches forward/reverse and 100,000 random reads; all 182 audio renders
match the raw build. Split-render tests include SAMPLE/GRAIN sharing factory
material across threads, seven scenes and eleven splits, delayed/lost core jobs.
Packing adds cache-hit checks and block decoding on misses; it has a real CPU/RAM
cost. Host timing is indicative only; target load and worst-case audio still need
hardware audition. Per-sample math helpers remain explicitly inlined. The static
budget recognizes cache-miss calls and SLICER's per-step timing call; USB playback
code is unchanged, although code layout changes its nested-loop cost heuristic.

Cold UI/protocol functions use upstream size annotations with explicit function
boundaries. The previous optimization checkpoints follow as historical evidence.

---

# Firmware optimization

## Upstream integration

The combined feature set fits the existing application slot: image 581,060 bytes
(504 free), ordinary static RAM 94,500/98,304, pool 333,900/344,064.
Fully overwritten scratch uses 8,800 bytes beside the flash driver, leaving
12,820 bytes there with an enforced 8 KiB reserve. Stack guards are unchanged.
The earlier measurements below describe a completed prior pass, not current margins.

Identical packed font glyph bitmaps now share existing offsets: data drops from
5,376 to 3,952 bytes: 816 from identical glyphs and 608 from exact byte-aligned
suffix/prefix sharing, with no renderer change or runtime decoding
work. Every raw BDF glyph and rendered uppercase/Latin-1 character remains identical,
including clipping, scaled titles, advances and anti-aliased-font compatibility.
After LOFI chord expansion, modulation application and move merging, the image is
432 bytes smaller than the initial gate-aware build. PR 111 is not integrated; see its
[separate assessment](PR-111-ASSESSMENT.md).

- Factory drums use a lossless dictionary: 512 sounds share 242 identical 21-byte
  parameter bases, with each sound’s final level byte stored separately. Raw factory
  bank storage drops by 5,158 bytes before decoder code. User-kit `dsnd_t`/`dkit_t`,
  SYN bank serialization and factory downloads remain expanded and unchanged.
- The dictionary bases use generated per-field bit widths: 132 bits stored in
  17 bytes per base, rather than 21 bytes. This saves 968 data bytes. The target
  decoder is 122 bytes versus the old 58-byte copy routine (+64), with a 21-byte
  width table. An otherwise identical target comparison saves 880 bytes after
  all code/table/alignment costs. It adds a bounded 21-field unpack on factory
  synth-drum hits; actual hardware timing has not been measured.
- A factory hit decodes only the selected sound into its voice’s stable 22-byte
  record. Six drum voices cost 132 bytes; control and sample rendering consume the
  ordinary record, without dictionary lookups on every sample. Editable SYN voices
  continue to reference their ordinary user-kit records.
- All 512 reconstructed records match a frozen pre-change byte hash. Host tests
  also exercise simultaneous factory voices, all 32 factory downloads, user kit
  round trips and 2,000 randomized user kits. All 182 reviewed renders match, with no health/routing/crash failures. The LOFI
  arp fixture update preserves input velocity; a fixed-100-velocity counterfactual
  reproduces upstream exactly.
- The default startup shows the firmware title instead of the bitmap splash,
  avoiding its 4,635-byte RLE data plus drawing code. The bitmap remains available
  as a build option when capacity permits; no instrument or sound bank is removed.
- Size-oriented compilation is paired with explicit hot-helper
  inlining. All 25 target loop-call caps and reviewed static budgets pass, and all
  182 audio fixtures match. Four layout-dependent budgets were refreshed after
  comparing raw loop instructions and call counts with -Os, without added hot
  calls. Physical-model primitives are also explicitly inlined: six additional
  PHYS/NOISE functions are guarded, with no per-sample helper calls. Modal
  branch weighting was reviewed against its smaller raw instruction count.
  Host timings/static checks do not prove the hardware audio deadline.

## Earlier completed branch pass


This pass preserves the branch's instruments, sample bank, starter libraries,
performance controls, project compatibility and splash screen. It concentrates on
shared storage and small compiler boundaries rather than changing the audio engine
or introducing compression/caches.

## Measured memory

Baseline: commit `d45c21c`, rebuilt in isolation with the same generated assets,
JieLi toolchain and `FELUCCA_SLICE=0` configuration as the optimized tree.

| Region | Before | After | Capacity | Saving / remaining |
| --- | ---: | ---: | ---: | ---: |
| Application image | 577,872 | 577,056 | 581,564 | 816 saved; 4,508 free |
| Static RAM (`.data + .bss`) | 97,748 | 96,212 | 98,304 | 1,536 saved; 2,092 free |
| Pool | 333,948 | 333,948 | 344,064 | Unchanged; 10,116 free |

These are linker measurements in bytes, not claims about runtime stack headroom.
The padded updater package size does not measure application code growth.

## Changes

- Lissajous history shares the existing waveform/note-trail union. Its stereo
  snapshot uses the FFT scratch arrays, which are idle in that style. No additional
  pool memory is allocated. Entering trails or Lissajous clears their history;
  subsequent frames retain it. The UI alone owns these buffers; audio writes the
  separate scope ring as before. Saving: 1,536 static bytes.
- Drum and musical starters call one metadata-reset helper for fills, locks,
  length, division, swing and activation. Their existing undo capture still happens
  before replacement. Isolated target saving: 96 image bytes.
- The musical starter draw/input, drum browser input, layer draw and visualizer
  update functions stay out of the large main UI handlers. Explicit `noinline`
  annotations are limited to measured UI wins; no audio/DSP function receives one.
  Their combined saving is measured above, rather than adding isolated estimates.
- Musical scale-degree mapping counts set bits instead of testing all twelve
  chromatic positions. Diatonic and pentatonic counts take seven/five iterations;
  the final image size is unchanged by this individual change. No cached state or
  parallel scale table is introduced.

## Verification

The target link, RAM/pool bounds, HAL register-access check and call-free 925-
instruction RAM code check pass. All 18 tracked DSP/ISR target-cost records match
the isolated baseline exactly, including audio ISR cost 37,891. These static
estimates do not establish a physical audio deadline margin.

All fourteen visualizer captures are byte-identical to the previous production
framebuffer fixtures. New regression assertions check Lissajous snapshot integrity,
history retention and transitions through note trails and waveform modes. A separate
pitch oracle enumerates every native scale, root and degrees across octave/MIDI
clamping bounds. Existing integrated starter tests check complete metadata undo,
track isolation and preview ownership.

Fresh trap-instrumented divide-by-zero checks cover UI fuzz, sequencer, project
storage and one minute of random live use. The installed Zig compiler lacks the
UBSan reporting runtime, so these builds use `-fsanitize-trap=all`. The suite now
refuses to run stale sanitizer binaries after any compilation failure.

The broader stress test's reference font decoder still assumed four-bit assets.
It now builds its independent enlarged bitmap in the generated header's one-bit
or legacy four-bit format. The corrected oracle compares 153,600 pixels with zero
differences and the 40,000-frame stress rerun passes.
An additional 40,000-frame run with undefined-behavior traps (excluding the DSP's
intentional signed-wrap/shift idioms) also passes. AddressSanitizer is unavailable
in this compiler. The Windows Node web protocol/sample/updater suite passes.

At this earlier checkpoint the full suite was **not globally green**: its LOFI/8BIT_ARP golden mismatch
and historical ISR budget failure reproduce on the untouched baseline. Both trees
produce the same 182 audio render hashes (including that LOFI mismatch), with zero
health, routing or crash failures. Goldens and budgets were not relaxed. Host
timing warnings are informational, not physical deadline measurements.

The full host-suite result is recorded in the integration verification document.
No physical FM1 or phone verification is claimed for this pass. The optimized build
is packaged as **2.5 Merthsoft.15** in the local installer; on-device installation
remains a user action.

## Rejected or deferred approaches

Individual target builds showed that forcing note-move helpers, groove drawing,
hold drawing, starter protocol handlers or all six added visualizers out of line
can increase code size. Blanket compiler annotations are not an optimization rule.

Project-load recovery and legacy import buffers remain independent: a failed import
must not overwrite the user's undo snapshot. MIDI ownership retains its current
capacity and release tracking. Sample data dominates ROM; lossy changes, deleting
presets/features or altering persistent formats are outside this pass.

Changes are confined to branch modules and a few UI function declarations. Future
upstream merges need no replacement linker script, compiler-wide flags or rewritten
DSP engine. Remeasure annotations after compiler changes or major UI edits.

## MerthSloop: independent PR 111 reductions and live effects

MIDI packing, DRAWBAR arena storage, shared main-loop/OTA scratch and XIP preset
banks recover 12,464 static RAM bytes before new effects, plus 1,024 pool bytes.
Wah/pan share the existing modulation clock/structure and add 64 runtime bytes.
No project format, sound-bank content or polyphony capacity is reduced.

Cold UI boundaries (`draw_graph`, `draw_menu`, `layers_input`) saved 592 measured
image bytes in this pass versus letting them inline into the large UI functions.
No DSP loop was moved out of line; annotations that saved nothing were removed.
The final branded image is 581,420 bytes and static RAM is 82,100 bytes.
Dual-core and overlapping low-memory relocation are deferred; see
[PR adoption assessment](PR-111-ASSESSMENT.md).

## Independent tie foundation (development)

Per-note sustain adds no track/project storage. Keeping MIDI entry, physical step
entry, groove drawing and column drawing out of line recovers 592 measured target
bytes (224 + 16 + 32 + 320). Trials that grew code or saved nothing were reverted.
The playback foundation's measured image was 581,464 bytes. Static RAM was
82,100 bytes and pool RAM
332,876 bytes. The target checks pass with 100 flash bytes free. Native editing and
recording/import integration were still development work at that checkpoint.
See [independent ties](PER-NOTE-TIES-DESIGN.md).

## Native independent ties and preserved playing preferences

The native editor and voice-mode preservation build measures 581,448 bytes, with
116 flash bytes free. Static RAM is 82,116 bytes; pool RAM remains 332,876 bytes.
Native controls use two UI bytes and existing undo/project storage, with no new
per-track or pattern arrays.

Exact icon-row sharing reduces the atlas from 3,096 to 1,755 data bytes (1,341
saved), retaining all two-bit alpha levels. The renderer performs a row lookup
when drawing UI icons, without an audio-path decoder or a decompression buffer.
Generation falls back to the original layout if a new atlas has over 256 rows or
would grow. PNG-oracle and C-renderer tests compare every icon, color and clipped
position in both formats: 1,761,280 pixel comparisons each.

Packed cold ROM lookups remove alignment padding from icon maps and the preset
browser (whose two small indices share a byte). The compiler handles packed
pointer accesses; these tables are neither persisted nor used in DSP loops.
Additional measured UI outlining saves 560 bytes: layer knobs 64, parameter
editing 224, piano roll 32, footer 16, tracks 192 and recording controls 32.
Shared note-slot deletion avoids duplicate compaction code. Target checks pass
without deleting presets, icons, visualizers or instrument functionality.

The subsequent native tie display audit measures 581,512 image bytes (52 free),
82,116 static RAM bytes and 332,876 pool bytes. This includes all-voice legacy tie
rendering, complete pitch cache keys, unobscured STEP roll and anchored LEN fixes.
No new persistent state or audio processing is introduced.

Custom progressions fit by sharing the plain/voice-led sequence renderer, storing
four ROM degrees in twelve bits with a seventh flag, using bounded scale walking,
and keeping the visualizer dispatcher out of the main UI draw function. Factory
starter output matches the previous renderer in 100,000 deterministic cases.
The custom build is 581,560 bytes (4 free); static RAM 82,116 and pool 332,876 bytes.
Unused layout/inlining trials were discarded. No audio DSP or saved format changes.

The eighth-swing build replaces seven always-64-bit masks per groove with aligned
16/32/64-bit arrays. ROM descriptors plus masks total 736 bytes versus 1536; the
accessor is 38 target bytes, with small alignment/caller changes measured in the
complete image. No decoding occurs on ordinary drum hits. The full build with
swing UI, preview, micro/gate/ratchet corrections measures 581,360 bytes (204 free),
200 fewer than the custom-progression build; static RAM 82,116 and pool 332,876 are
unchanged. The old groove definitions' frozen hash verifies every mask and length.

Explicit section saves also refresh the existing startup object. This adds 144 image
bytes without RAM/pool growth and preserves the upstream project/storage format.
Queued section writes bypass routine settings idle delay once stopped and quiet;
failed writes retain their retry delay.

## Unused parameter icons after the NEON overhaul

Same-source/version target comparison against `9fd9ddc`: **584,336 → 581,296 B**,
recovering **3,040 B** in the 584,336-byte application slot. Main static RAM
89,128 B, low RAM 28,280 B and pool 332,876 B are unchanged. This is a private
development build; the served .49 installer has not been replaced.

NEON cards have full-width labels and do not draw parameter/engine icons.
However, they still called the old descriptor/value-to-icon lookup and put
the result in their redraw cache. Those calls kept the label map, waveform
map, engine map and their lookup code alive despite having no visible output.
Removing the obsolete card argument/cache component lets that code disappear.

Only the tempo and tape icons are actually drawn, both in the header. The
generator now emits a separate compact two-icon atlas from the same PNG;
production uses it, while the complete atlas remains available for asset and
renderer tests. The atlas payload is **66 B versus 1,755 B**. That raw-data
difference is part of the **3,040 B total linked saving**, not an additional
saving to add to it. The compact IDs are private render indices, not stored
song fields or MIDI/protocol IDs. No artwork, visible control or musical
feature is removed. No audio decoder, delay buffer, DSP stage or storage
format is changed.

Validation: **113 before/after framebuffer captures are byte-identical**,
including regular pages, held layers, settings and redraw transitions. The
actual UI's 20,000-frame fuzz, shared HOME/entry-context tests and redraw
checks pass. Generator tests compare the compact atlas against the original
PNG, reject invalid/repeated subset names, and retain full-atlas row-packing,
fallback and missing-artwork coverage. The actual C renderer checks 40,960
header-atlas pixels and 1,761,280 pixels each for full packed and legacy atlas
formats, including clipping and alpha. Target placement and DSP loop budgets
pass. Evidence is under ignored `build/space-50/`.

This is primarily a MerthSloop cleanup: upstream still draws parameter icons,
so deleting their mappings there would remove visible behavior. The generally
useful lesson is to trace retained assets through their actual draw calls;
the same 3,040-byte saving is not claimed for upstream.

## Combined review integration

The versioned 2.7.1.50 image measures **579,744/584,336 B**, leaving **4,592 B**
flash free. Main static RAM is **86,128/98,304 B**, low RAM **28,280/29,496 B**,
and pool **332,876/344,064 B** with the existing 8,192-byte reserve. This is
**1,552 fewer image bytes and 3,000 fewer main RAM bytes** than the 581,296-byte
post-icon baseline, despite the additional correctness fixes. It is a real
combined build, not the sum of independent audit deltas.

Accepted structural changes are explicit cold punch-state initialization,
deletion of the bypassed pre-NEON TRACKS columns, direct legacy project conversion,
generator-backed ragged preset trims and scalar uniform font metrics with
proportional fallback. Legacy formats, original trim values, all font pixels,
factory samples, saved format IDs and direct playback remain supported.
The additional boot-expanded sample-zone/numeric-table experiments remain out.

The shared punch startup helper is called after target BSS/data initialization,
once in the common host bootstrap, and by preview initialization. It is not a
per-block lazy initializer. Target RAM-code placement, all 24 monitored DSP/ISR
budgets and hot-helper call caps pass. The audio fixes deliberately widen active
FREEZE/crossfade arithmetic; they are not claimed to have zero instruction cost.
The audio specialist's paired target has unchanged mix-loop calls/divides and
12 additional disassembled mix instructions. Hardware deadlines are unmeasured.

Parent integration evidence is in ignored `build/integration-51/`: versioned
target/loader build, 47 focused firmware cases, 182 audio goldens after eight
explicit fine-pitch corrections, 40,000-frame mixed UI/audio stress, native/WASM
parity, actual package loader/OTA and maximum-capacity checks. Of 113 baseline
screen captures, 110 remain byte-identical; the three sampled-audio visualizers
change with the corrected pitch output. The stale independent-tie display test
now asserts all four connected note bars over the current piano-roll viewport,
plus its existing native editing/undo invariants, rather than old graph geometry.
See [review synthesis](../reviews/REVIEW-SYNTHESIS.md) and the specialist reports
for individual tests, isolated measurements and remaining physical validation.

## Controls and live progression follow-up

The combined 2.7.1.52 target is 582,048 / 584,336 B (2,288 B flash free), with
86,144 / 98,304 B main static RAM, 28,280 / 29,496 B low RAM and
332,876 / 344,064 B audio pool. Compared with the combined review .50 image,
these controls/correctness/performance additions consume 2,304 B of application
space and 16 B of main static RAM. No audio buffer, sample-content cut or runtime
sample decompression was added. Shared chord pitch/modifier rules are reused by
ordinary chords and progression performance instead of maintaining a second
implementation. Existing recording and note-chain helpers carry the continuations.

Final validation: 50 focused firmware programs pass; the corrected sequencing
cases were recompiled/rerun after the quantized-record skip fix. The expanded
progression suite also passes optimized/UBSan tests. All 182 golden renders are
unchanged, with no health, voice/routing, CPU-budget or crash failures. All 24
monitored target DSP loops/inlining checks pass, and 32 native/WASM audio profiles
match exactly. These checks do not measure physical deadline/stack high-water or
replace hardware feel/sound validation.

## Appended instrument preset pack

Fourteen data-only factory presets increase the preceding 582,048 B target to 582,424 B: **376 B flash** and **504 B low RAM**. Flash free is 1,912 B; low RAM free is 712 B. Main static RAM and the audio pool are unchanged. No feature removals or new render paths were required. See [preset pack measurements and validation](INSTRUMENT-PRESET-PACK.md).

## Selected upstream integration completion

Partial held-note recording, SF_LEG playback/STEP transport, preset arp ownership and the HOME section badge add **544 B flash** and **32 B main RAM** to the instrument-pack build. Target: **582,968 / 584,336 B**, leaving **1,368 B**. Main static RAM is 86,176 B; low RAM and pool remain unchanged. Existing render algorithms and all 196 audio golden hashes are unchanged.

## LOFI experiments

CASST/SMPLR reuse the existing stereo history, filter state and shared coloration code. Four additional presets cost 144 B low RAM. Combined target increase: **380 B flash**, leaving **988 B**. Main static RAM/audio pool do not grow. Sampler coefficients, including drive compensation, are calculated per block only while entering/using that mode. No features were removed.

### Tape/sampler refinement

Consolidating cassette into TAPE and blending sampler controls progressively recovers 24 B net: **583,324 / 584,336 B**, **1,012 B free**. Static RAM totals are unchanged. Sampler bypass/low-control tests now use loud internal bus input to catch unintended clipping.

The sampler drive follow-up fixes above-unity fixed-point gain and squares its control curve: **583,388 B**, leaving **948 B flash**; RAM unchanged. This is a correctness/range fix costing 64 B, not an optimization saving.

### First ambient headroom integration

Integrated the insert-state union, upload/backup decode staging in main scratch,
and FFT imaginary vector in the existing second scratch vector. Used the portable
C constant-bound clamp specialization with original dynamic/inverted fallback.
Moved the exact GR_HANN/FM6_STATICS literals into the normal table generator and
boot decoder list; factory preset fields now pack by per-column bit width before
existing LZ4. The dedicated zero preset fallback moves to BSS.

Combined target: **581,996 / 584,336 B**, **2,340 B flash free**; a measured
**1,392 B reduction** from the 583,388 B baseline. Main static RAM is
**86,320 / 98,304 B** (144 B more overall, 11,984 B free). Low RAM remains
**28,928 / 29,496 B** (568 B free). Pool is **331,852 / 344,064 B**: 1,024 B
recovered, 4,020 B above the unchanged 8,192 B reserve. No playback decoder,
new samples, removed functionality, saved-format change or analog loop sharing.
The boot table list now restores 22,032 bytes from 7,969 payload bytes, plus
its existing separately packed FIR; restored factory payload remains 6,156 B.
Individual audit deltas were not additive; these are actual combined-link values.

Validation: 200 unchanged goldens in each raw and boot-restored build; 171
complete raw/restored presets; 22,032 byte-exact restored table bytes; 2,003,375
clamp contract comparisons; frozen 160,000-block insert transition oracle;
poisoned upload/backup staging with eight uploads across all slots and invalid
header refusal; FFT scratch poisoning/waveform preservation; UI redraw parity
and 20,000-frame fuzz; delayed two-core rendering across seven scenes and eleven
splits; arena switching, sampler mechanisms and native/WASM parity. Target budgets
and 24 hot-loop call caps pass unchanged. Host evidence does not establish
hardware stack high-water, boot time or worst-case audio deadlines.

Assembly clamps, shared analog rendering and preset relocation remain separate
audit candidates. Larger zone/name startup packing is now integrated; the
current cold-state conversion regresses flash and is rejected.
See [the reconciliation](AMBIENT-HEADROOM-SYNTHESIS.md).

## Extended metadata startup packing

The next measured batch reaches **582,804 / 584,336 B**: **1,532 B free**,
1,196 B below .63 including the requested independent REV HOLD removal. The
retained data/code optimizations add 2,976 B main RAM, leaving 8,896 B there,
with low RAM and pool unchanged. They restore zone metadata and compact factory
names at startup, share chord scale-walking code and remove a duplicate UI
dispatch. No new work is added to sample playback. The old cold-state candidate
is rejected after a current-source A/B grows flash by 64 B. See the
[measurements and checks](AMBIENT-HEADROOM-SYNTHESIS.md#second-integration-metadata-packing-and-small-source-sharing).

## Broader build-time LZ4 match search

Startup-table and preset metadata packers now use `compress_best`: a bounded
64-candidate hash-chain search indexes positions skipped by earlier matches,
with one-position lazy matching. Each block falls back to the original fast
compressor if that output is smaller. This changes only the Python build step;
the device's existing LZ4 decoder, bootstrap order and restored destinations
remain unchanged. Loader compression still uses the original compressor.

Same-version target A/B: **582,980 → 582,212 B**, recovering **768 B flash**.
The final .66 also removes opaque HOME lane backgrounds and has the same size:
**2,124 B flash free**, main static 89,408 B, low 28,928 B, pool 331,852 B.
Startup-table payload falls 7,969 → 7,338 B; preset fields 2,625 → 2,542 B;
zone metadata 854 → 798 B. These gross payload deltas are not individually
additive linked savings. No additional playback decoding or sample compression.

Validation: identical pre/post original table and zone oracles, 22,032 restored
table bytes plus FIR, all 171 full raw/restored preset records/names, startup
state/zone byte comparison and decoder boundary guards. All 200 audio goldens
are unchanged with no health/voice/routing/CPU-budget failures. UI tests verify
full/incremental redraw parity, palette rendering, intact track numbers/notes,
and visible beat-driven grid pixels inside former opaque lane panels. Target
DSP budgets and all 24 inlining caps pass. Python tests exercise deterministic
output, actual improvement over the fast compressor, fallback size guarantees,
overlapping matches, offsets/window edges, literal/match extension boundaries
and the standard LZ4 terminal-literal constraints. Build-time search increases
Python work; hardware boot timing is unmeasured.

### Minimum-cost startup block parsing

The next isolated audit finds a further **112 B** net at the frozen .67 baseline
by choosing a minimum-byte LZ4 parse for metadata blocks up to 8 KiB. It retains
the old lazy/fast fallback, block decoder and restored bytes. The tradeoff is
more Python work during generation (about twelve seconds in the audit), with
no playback decoding or target RAM cost. The candidate is integrated in .68;
the combined sustain/synced-EVOLVE image is **584,064 B**, leaving **272 B**, with
main static RAM 89,616 B. Individual audit deltas are not additive substitutes
for this complete link. See [evidence and patch](SUSTAIN-HEADROOM-AUDIT.md).

### Cold sharing and voice admission integration

The actual .68 → .69 combined target shrinks **584,064 → 583,776 B**, recovering **288 B** and leaving **560 B free**. Main static 89,616 B, low RAM 28,928 B and pool 331,852 B remain unchanged. This includes project/settings/storage/editor cold sharing, weighted admission, FM6 eight and a new steal-fade retrigger admission fix. Do not substitute isolated audit deltas for this link. Startup strings remain in flash; no new playback decoder or sample packing. The maintained `headroom_commit_test` exercises real writes/restores, malformed records, A/B fault recovery, busy guards and WATCH updates. `voice_budget_test` proves capacity and catches fading-slot readmission bypasses.

### Sustain characters within the remaining budget

PIANO/STRINGS/LOFI, DAMP and AMOUNT add **428 B flash** and **320 B main static RAM** after the cold-sharing/weighted-voice build: final **584,204 B**, **132 B flash free**, main static **89,936 B**. Per-voice tail tone state avoids borrowing engine state; one 128-byte per-render stack block isolates released-note processing from clean new attacks. No persistent history buffer, sample decoder or saved-format change. Counting admission per track reduces repeated engine-cost lookups without changing the final linked size. Hardware stack/deadline measurements remain open.

### Adjustable admission and drone recovery

The complete .71 experiment is **584,280 B**, **56 B flash free**, main static **90,336 B**, **7,968 B free**. Relative to .70: +76 B flash / +400 B main static. It includes optional 30-unit admission and a 384-byte per-pitch pending map, plus common victim selection. The previously audited ordered clamp now uses two native signed min/max operations in a HAL helper; dynamic/inverted bounds and host/WASM remain C. Its early-clobber register contract is checked using the actual target output. No ROM checking rule, DSP budget, sample decoding policy or physical slot cap is relaxed. Full hardware timing/stack qualification remains open. See [experiment](VOICE-BUDGET-EXPERIMENT.md).

The .72 selector extension adds three higher budgets (36, 42, 48 units). Sharing the bounded menu stepping with USB RATE keeps its net cost to 20 B flash and no RAM: 584,300 B, 36 B free. No DSP loop or engine cap changes.

The .75 diagnostic scheduler trial reuses `track_render`'s existing per-part count rather than adding a voice scan each block. A 12-byte cache is read only after the preceding worker join; eligibility checks and winner selection share one bounded loop. The compact ABOUT counters fund the experiment without musical feature cuts. Diagnostic target 584,316 B / main 90,368 B; normal remains 584,300 B / main 90,336 B. Relative engine weights are heuristic and device deadline qualification remains open. [Details](VOICE-BUDGET-EXPERIMENT.md#core-balancing-trial-after-the-longer-hardware-run).

The combined .76 About restoration, policy promotion and EVOLVE bounds target is 584,296 B (40 B free), main 90,336 B. About credit rows share one draw path; native runtime fields share metadata/address resolution; keyboard and evolution handlers retain explicit call boundaries. No sampled audio, preset, musical function or cold-start behavior is removed. A remeasurement of the older cold-state initialization candidate enlarged this source and was discarded; original boot contracts remain intact. Report the actual combined link, not frozen audit deltas.


### Track snapshots and shared ANALOG renderer

The .77 saved-budget build is 584,328 B (8 B free). The combined .78 snapshot feature and shared ANALOG body link at **583,864 B (472 B free)**, a **464 B net reduction** including the new feature. Main static RAM is **91,216 B (7,088 B free)**; low RAM 28,928 B and pool 331,852 B are unchanged. No per-hit decoding or new per-block morph runner is added. The shared sample loop has zero helper calls and weighted cost 640 against the original 638 budget (+0.3%, static estimator only). Hardware cycles/stack are not qualified by this comparison. All 200 audio goldens, snapshot/lock sanitizer checks, dual-core/timeout parity and UI navigation/fuzz pass. See [track snapshots](TRACK-SNAPSHOTS-DESIGN.md).
