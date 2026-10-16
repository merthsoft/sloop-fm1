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
