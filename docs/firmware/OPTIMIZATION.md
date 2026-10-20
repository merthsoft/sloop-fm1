# Current effects build

MerthSloop 2.7.1.33: **581,176 image bytes**, **388 free**, main static RAM
**85,656/98,304**, low RAM **22,772/29,496**, pool **332,876/344,064**.
VINYL reuses its existing ring; stronger modulation and SPRING feedback each add
one small smoothed/control value. DOUBLE TIME removes the former octave shifter.
See [effects implementation](EFFECTS-IMPLEMENTATION.md) for integration and limits.
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
