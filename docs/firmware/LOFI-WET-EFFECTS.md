# Integrated delay characters and wet-color research

## Current native integration

FX → DLY TYPE maps its remaining knobs by type: DIGI TIME/FDBK/MIX,
TAPE or DUB DRIVE/TONE/MIX, CRUSH BITS/RATE/MIX. DIGI retains the original delay;
TAPE applies adjustable soft saturation before feedback filtering and return;
DUB uses a stronger soft-clip curve and a high-pass that removes low-end buildup
from successive repeats, alongside the existing dark low-pass on the first
repeat and feedback. DRIVE has an earlier concave response across its full
range. Its high-pass reuses CRUSH's mutually exclusive held-sample state. A small
mode selector fades through DIGITAL before changing algorithms and primes the
held state at the transition, avoiding a sample-hold startup click. No extra
audio buffer is added.
CRUSH offers 4–16 bits and approximately 2.8–44 kHz sample holding.
The preceding DLY page keeps TIME/FDBK/COLR/MIX for every type. These are global bus settings,
independent from master LOFI, which now defaults to OFF.

Production uses the compact Q9 delay-only helper in fx.c, including a settled
DIGITAL fast path. It does not use the larger reusable wet_color.c module below.
The logical helper state is four bytes; complete static RAM grows 16 bytes.
No new audio buffer or sample decoder is added. A DLY TYPE action-word tag in
G_ENGSEL is adopted from raw storage before clamping, with untagged/unknown
values defaulting to DIGI. Default project bytes retain upstream compatibility.
Other markers, project size and checksum remain unchanged. Phone song archives
and browser backups preserve the new signed tag byte-for-byte in tests.

Earlier delay integration target: 580,312 image bytes in a 584,336-byte slot, 4,024 free;
main static RAM 89,080/98,304, low RAM 28,280/29,496, pool 332,876/344,064.
See [architectural headroom](ARCHITECTURAL-HEADROOM.md) for the preset-storage
trade and capacity arithmetic; neither adds playback sample decoding. Target image safety and DSP budgets
pass. No physical deadline or startup-duration measurement is claimed.

Tests cover exhaustive int16 tap bounds, exact DIGITAL identity, CRUSH grid,
mode changes, maximum-feedback send/tempo/time/type automation, four modes'
project and section recall, invalid tags, reverb/LOFI/drum metadata coexistence,
actual native page controls and 32 native/WASM profiles. Original DIGITAL WAV
bytes match exactly; all 182 existing golden audio renders are unchanged.
The canonical upstream project fixture and 25 phone song-library checks pass.
A web fixture parser now anchors FM6 symbol names so BOOT_FM6_FACTORY cannot be
mistaken for the uncompressed FM6_FACTORY oracle.

TAPE wow/flutter, stereo/ping-pong delay, and reverb return coloration remain
future work. The research and controlled measurements below are retained as
historical design evidence, not a list of unintegrated requirements.

## Reusable wet coloration prototype

`firmware/src/wet_color.c` is a standalone include-style fixed-point module,
matching the firmware's DSP organization. It changes no existing audio path or
project format. Include it before its callers; instantiate zero-initialized
`wet_color_t delay_color, reverb_color;` in the integrating source.

Call `wet_color_frame(&state, frame, channels, mode, amount)` once per audio
frame. `frame` contains one or two int32 samples, nominal full scale +/-32768;
channels must stay fixed for an instance. Use channels=1 for the existing mono
delay and channels=2 for the stereo reverb. Valid modes are WET_COLOR_OFF,
WET_COLOR_TAPE and WET_COLOR_CRUSH. Amount is 0..127 (larger values clamp).
An invalid mode requests OFF. Invalid channel counts do nothing. State and
control snapshots belong to the audio thread; do not race UI writes against it.

TAPE blends an odd quadratic soft knee and a fixed one-pole low-pass (coefficient
1/8, approximately 940 Hz at 44.1 kHz). The intentionally dark maximum setting
compresses 24000 to 15211 and limits saturated output to +/-16384. CRUSH blends
nearest-rounded 8-bit quantization (step 256, signed symmetric grid including
both endpoints +/-32768) with a four-frame hold (11.025 kHz at 44.1 kHz).
Amount controls the blend, not separate bit-depth/rate parameters. Stereo holds
share timing but retain separate samples and filter history. No mono fold, RNG,
noise floor, mutable master VINYL state, or extra audio ring is involved.

Each mode weight slews by 64/32768 per sample: a full change takes 512 samples
(11.6 ms at 44.1 kHz). Changing modes crossfades their weights without resetting
history. OFF/amount zero fades out, then returns every int32 sample unchanged
and clears history; already-settled OFF is exact bypass immediately. Immediate
exact bypass at the instant of a live switch conflicts with click suppression;
the transition deliberately takes precedence until the fade ends. This limits
control-induced steps, not arbitrary discontinuities already present in audio.
Integer low-pass updates round toward the target so silence reaches exact zero.
This nonlinear processing is deliberately not oversampled; aliasing is part of
CRUSH and remains a limitation of the lightweight TAPE knee.

## Integration placement

For progressive tape wear, color the delayed tap **before it is multiplied by
feedback and written back to the existing delay ring**. Return that colored tap
to the wet mix as well if desired. Call the instance once per frame and reuse
its result for feedback and return; processing it twice advances hold/smoothing
twice. Keep the fresh send separate if only repeats should wear down. The knee
is non-expansive, but existing feedback clamps/limits remain necessary. CRUSH
can round low signals upward and sustain small feedback limit cycles; avoid
unity feedback/freeze with CRUSH in the loop unless that sound is intentional.

For minimum-risk reverb integration, color the final L/R wet return before its
mix into dry, with one stereo instance. This preserves all existing stereo
topology, but coloration will not accumulate inside the tail. Moving it inside
an FDN or spring feedback loop changes tuning/stability and requires separate
review and per-node state; do not share this stereo state across internal taps.
Consider keeping reverb freeze independent of return coloration.

Wow/flutter is intentionally absent. Add bounded modulation to the existing
delay tap controller and retain its fractional interpolation/time-change
crossfade. Combining pitch modulation with an independent new delay line would
cost RAM and create competing time controls. Its depth/rate should be reviewed
with the current tap controller before adding a native control.

## Native controls and storage recommendation

Keep only COLOR (OFF/TAPE/CRUSH) and AGE (0..127), independently for delay and
reverb. Leave the reverb type menu and master VINYL controls unchanged.
Prefer two bytes per bus in a **versioned optional extension**: mode byte 0..2
and amount byte 0..127; absent fields default to OFF/0, unknown mode to OFF,
and excessive amount clamps. If bytes are scarce, one byte can encode OFF=0,
TAPE=1..127 and CRUSH=129..255, reserving 128; this reduces amount resolution
or omits a redundant active-mode zero state. Two bytes are clearer. Do not
repurpose existing parameter IDs or reserved bits without auditing old readers,
project size checks, editor transport and round trips. No format change is made
by this prototype.

## Validation and cost

Standalone test: `tests/wet_color_test.c`, always assertions-enabled, no
hostsim/generated-header dependency. Compile with a C99 compiler, for example:

```sh
cc -O2 -UNDEBUG -Wall -Wextra -Werror tests/wet_color_test.c -lm -o build/lofi-wet/wet_color_test
build/lofi-wet/wet_color_test
```

Covers full-int32 exact settled bypass, overload bounds, zero decay, DC
symmetry, compression, measured high-frequency loss, quantizer grid/hold,
stereo independence, live mode changes, bounded constant-input transition
steps, deterministic replay, and independent mono/stereo instances. There is
no block API: per-frame state makes arbitrary block subdivision equivalent.
Host TAPE 10 kHz/200 Hz power ratio measured 0.010845 at amplitude 2000.
No hardware capture or listening claim is made.

State is 28 bytes per instance (56 for delay plus stereo reverb). No allocations
or tables. Active stereo performs two knee multiplies, four 64-bit blend
multiplies and inexpensive fixed-divisor arithmetic per frame; both algorithms
currently run whenever either mode is active to keep switching predictable.
Settled OFF skips DSP. Target compiler lowering of signed 64-bit arithmetic
must be checked; no target cycle/flash claim follows from the host tests.
Expect a small source integration (include, two states, two call sites) plus
UI/persistence work. Allow roughly 1–2 hours for hook review and host regression,
with target profiling and hardware listening budgeted separately. Do not ship
until final firmware flash/cycle budgets and feedback behavior are measured.

## Latest rebase: .38 compact delay candidate

The new frozen snapshot under `build/lofi-wet/delay-next/baseline38/` reproduces
the parent's production .38 baseline exactly: **580708 bytes, 856 free**.
It includes the piano-roll grid, master LOFI OFF and reviewed UI clamp helper.
The final Q9 DIGITAL/TAPE/CRUSH candidate with the same native TYPE page and
tested mode-storage design measures **580972 bytes: +264, leaving 592 free**.
This supersedes the earlier 16-byte-margin result below. Its generated DSP
source hash is unchanged from the earlier sound/correctness experiment.

| .38 snapshot | Image bytes | Added bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| Unmodified baseline | 580708 | 0 | 856 |
| Compact Q9 DSP + native TYPE + storage | 580972 | 264 | 592 |

`.bss` grows from 88536 to 88552 bytes (+16); `.data` remains 524 bytes.
Pool, lowram, ring sizes and RAM-resident code are unchanged. Compiler options
remain the production options; there is no sample compression, font change,
feature deletion or production edit in this measurement. The original reusable
wet-color API is unchanged. Commands, section/symbol dumps and logs are in the
snapshot's variant directories; `baseline38/measurements.json` records results.

This candidate now fits the measured .38 slot with a useful, though still
limited, margin. The earlier suggestion to recover roughly 1 KiB was a
conservative integration target, not a hardware requirement. Final combined
target checks, live delay-time/tempo/send/throw regressions, CPU profiling,
hardware listening and editor persistence review remain integration work.
No package or deployment was produced from the private snapshot.

## Earlier delay pass: measured private candidates (.37)

All experiments below live under `build/lofi-wet/delay-next/`. Production DSP,
parameters, formats, fonts, sample packing, compiler flags and release assets
were not changed. There is no release/package output. `measure.py` compiles a
frozen firmware copy with the existing pi32v2 toolchain and production `-Oz`,
then links private ELF files using the existing startup objects. Its private
linker has a larger XIP region solely to measure overflow; reported margins
always use the real **581564-byte** limit. No oversized image is deployable.
Image size is `.text + .ram_text + .c1_text + .data`, rather than ELF file size.
Commands, compiler/link logs, section tables and symbols are retained per case.

The pre-master-OFF snapshot reproduces the served .37 baseline exactly:
581140 bytes, 424 free. It gives these measured whole-image costs:

| Candidate | Image bytes | Added bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| Unmodified snapshot | 581140 | 0 | 424 |
| Reusable wet module, mono DSP only | 581684 | 544 | -120 |
| Delay-only two-weight DSP, no UI | 581668 | 528 | -104 |
| Two weights + native TYPE page + storage | 581660 | 520 | -96 |
| One signed blend, DSP only | 581524 | 384 | 40 |
| One signed Q15 blend + TYPE + storage | 581564 | 424 | 0 |
| One signed Q9 blend + TYPE + storage | 581548 | 408 | 16 |

The parent then added master LOFI OFF. A second independent source snapshot
under `off-baseline/` measures **581252 bytes, 312 free**. This is distinct from
the earlier `integration-37/target-off.log` result of 581300 bytes; source
integration was still moving. Do not substitute either snapshot for a final
release build. The second snapshot measures:

| Candidate | Image bytes | Added bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| Post-OFF snapshot baseline | 581252 | 0 | 312 |
| Signed Q15 blend + TYPE + storage | 581708 | 456 | -144 |
| Signed Q9 blend + TYPE + storage | 581548 | 296 | 16 |
| DIGITAL/TAPE-only fallback + TYPE + storage | 581852 | 600 | -288 |

These are complete compiler results, not additive budgets. Smaller source or
fewer types did not always produce a smaller image. In the post-OFF snapshot,
`mix_block` grows by 148 bytes for Q9 three-character processing, versus 448
bytes for the tape-only candidate. No compiler flag or existing feature was
removed to obtain these results. The 16-byte-margin candidate is **not a ship
recommendation**. Reclaim and verify roughly 1 KiB of practical headroom before
integrating and remeasure the final combined source. Web/editor controls and
their compatibility audit are not included in this native firmware budget.

### Recommended narrow behavior and controls

Keep the existing DLY page's TIME / FDBK / COLR / MIX unchanged. Add one DLY 2
page with TYPE = DIGITAL / TAPE / CRUSH; the remaining columns can be empty.
No extra menu framework, preset list, separate drive/bit/rate controls, or new
audio buffer is needed. DIGITAL is the exact existing delay, including its
current feedback low-pass. COLR retains its existing coefficient mapping for
all types. MIX already controls the wet return, so a separate delay OFF type
is unnecessary for this first expansion. This is independent of master LOFI
OFF and does not reuse master LOFI state.

The private Q9 candidate (`small-color.c`) uses only two int16 state values:
blend and held sample. The blend slews one count per frame between DIGITAL=0,
TAPE=-512 and CRUSH=512. DIGITAL-to-character takes 512 frames (11.6 ms at
44.1 kHz); TAPE-to-CRUSH passes smoothly through DIGITAL over 1024 frames.
The intermediate DIGITAL sound is a deliberate compromise that saves a second
weight/multiply; it is not an equal-power crossfade. Constant-input control
transition steps were checked, not arbitrary input discontinuities.

TAPE applies `x - x*abs(x)/65536` to the existing bounded int16 delayed tap,
then feeds the existing feedback low-pass. It compresses progressively on
each circulation; return coloration makes the first repeat colored as well.
CRUSH quantizes toward zero to a 256-count grid (roughly 8 bits) and holds for
four samples. Hold cadence reuses `fx.dly_w & 3`, so no separate phase/ring is
allocated. Toward-zero quantization is intentional: unlike nearest rounding,
it cannot enlarge a small held sample merely by rounding it upward. Aliasing
and stepped quiet decay are part of this character. There is no added hiss.

Insert exactly one coloration call after the existing time-change tap
crossfade and before `fx.dly_lp` is updated. Use the colored `x` both for that
feedback low-pass and the existing wet-return calculation. Preserve the fresh
send addition, int16 write clamp, feedback coefficient/range, throw behavior,
TIME divisions and 64-sample time-change crossfade verbatim. The knee and
quantization are non-expansive on individual samples; the hold can retain a
previous peak briefly, but its output remains bounded by recent tap peaks.
The existing feedback ceiling is about 0.8914 (127*230/32768), and the low-pass
is a convex update. This does not justify raising feedback or adding freeze.
Existing fixed-point low-pass rounding can retain tiny DC remnants; no new
claim of perfectly silent infinite feedback decay is made.

Q9 arithmetic is only valid for the bounded delay tap, where differences times
512 fit comfortably in int32. It is not a replacement for the reusable wet
module's full-int32/stereo API. The logical delay state is four bytes; the
complete UI/storage candidate increases measured `.bss` by 16 bytes because
of the appended global, surrounding structures and alignment. `.data`, pool,
lowram and all audio-ring sizes are unchanged. Target CPU deadlines remain
unmeasured; fewer multiplies and no 64-bit blend are not a cycle benchmark.

Wow/flutter and stereo/ping-pong routing are deferred. Wow needs fractional
interpolation in the existing tap controller, plus coordination with its
time-change fade; integer tap jumps would click. Ping-pong needs routing and
feedback-topology review even if the physical ring is reused. Neither is
silently substituted for an existing delay capability to make this fit.

### Persistence experiment and compatibility boundary

The tested private mode-only encoding uses the currently unused on-disk
`g[G_ENGSEL]` action slot: zero for DIGITAL; signed int16 tag `0xED00 | mode`
for TAPE=1 or CRUSH=2. The raw stored word is inspected before normal global
clamping. Only the exact tag and modes 0..2 are accepted; absent, legacy and
unknown values default to DIGITAL. The negative tag clamps to zero in old
readers, which store engines per track separately; an old-reader resave loses
the new character. The project record length/checksum algorithm is unchanged.
This remains a proposed marker allocation, requiring parent/editor review.

**Do not use `g[G_SAVE]`: it already contains the drum tune/decay marker and
checksum.** The initial sizing sketch used that slot, but the format audit
caught its conflict and the final measured/tested sketch uses G_ENGSEL instead.
Likewise, G_LOAD already encodes expanded reverb and G_NAME/G_ENGGO encode
master LOFI, including the new OFF extension. No existing marker is reclaimed.
The original two-byte-per-bus optional-extension recommendation above remains
the clearer route for a later richer wet-effects format; it is unnecessary
for this three-choice delay-only prototype.

`tests/delay_storage.c` in the private directory passes assertions for all
three modes, full-project and section adoption, legacy/invalid defaults,
checksum validity and coexistence with reverb type/time, master LOFI values,
and drum tuning/decay. Native persistence passing does not prove phone/editor
round trips: audit their raw global handling and add tests before integration.

### Controlled audio and correctness checks

`render.py` builds the actual private `fx_buses` source with the existing host
harness. It renders the same 125 ms synthetic pluck (220/880/6000 Hz components)
through each character: 120 BPM, eighth-note tap (11025 samples), FDBK=100,
COLR=100, MIX=127. Files are wet-only, stereo dual-mono, four seconds, 44.1 kHz
PCM16, with identical gain and no normalization or master LOFI. They are
offline renders, **not hardware captures**, and no listening judgment is
claimed. `comparison-digital-tape-crush.wav` concatenates DIGITAL, TAPE, CRUSH
in that order, about four seconds each, with no inserted labels or gaps.

The DIGITAL WAV is byte-for-byte equal to the unmodified baseline render.
Both channel outputs are asserted equal for this existing centered delay.
The candidate passes an exhaustive int16 tap sweep for bounds, exact DIGITAL
identity and full CRUSH grid, plus bounded constant-input mode transitions
and exact-zero coloration after hold history is flushed. These tests exercise
the coloration and actual delay path, but do not replace target listening,
live TIME/tempo automation, overload/send/throw regression or CPU profiling.

| Character | First repeat RMS | Fourth repeat RMS | Eighth repeat RMS |
| --- | ---: | ---: | ---: |
| DIGITAL | 6472.66 | 2183.48 | 520.14 |
| TAPE | 5745.41 | 1699.78 | 389.65 |
| CRUSH | 6309.46 | 1849.02 | 193.94 |

These are measured sample-amplitude RMS values over each original burst's
window. They demonstrate different decay, not perceptual loudness matching.
Source, WAV SHA-256 hashes, metrics and private tests are retained alongside
`measurements.json`; the second snapshot has its own measurement report.

## Adjustable character controls

The integrated source adds per-character native controls. DLY TYPE maps:

| TYPE | Knob 2 | Knob 3 | Knob 4 |
| --- | --- | --- | --- |
| DIGI | TIME | FDBK | MIX |
| TAPE | DRIVE | TONE | MIX |
| CRUSH | BITS | RATE | MIX |

TAPE DRIVE sets the saturation blend, 0..127; settled zero is exact clean tap
identity. TONE uses the existing shared feedback COLR low-pass. CRUSH BITS is
4..16; RATE selects approximately 2.8/5.5/11/22/44 kHz, with higher positions
increasing fidelity. 16 bits and 44 kHz preserve the tap exactly. Original DLY
still exposes TIME/FDBK/COLR/MIX for every character. DRIVE defaults 127, BITS 8
and RATE 11 kHz; these match the preceding character settings closely.

G_DDRIVE/G_DBITS/G_DHOLD append after G_DTYPE. Their project markers use
g[G_CLRSEQ] = 0xD400|DRIVE and g[G_INITSND] = 0xD800|((BITS-4)<<3)|RATE.
The two words are GO actions, not live track properties. Missing/invalid tags
load defaults; default parameter values omit the markers. G_ENGSEL retains
the mode tag. The existing reserved bytes already hold TEXTURE and are not
reused; reverb, master LOFI and drum tuning markers are also unchanged.
Inactive types retain their parameters. Phone archives/web backups treat these
project bytes as opaque and preserve all markers. No record-size change.

LOFI UI order is now OFF/VINYL/TAPE/VHS/CD, default OFF. Stored/DSP IDs remain
VINYL/TAPE/VHS/CD/OFF so existing explicit choices do not silently change.
The selector maps through lofi_mode; UI labels and preview inputs follow the
new order. Tests include OFF-first navigation and saved old-style tags.

The real target now fits after startup-only preset metadata packing and
reclaiming unused capacity within the unchanged physical update boundary.
Native control/persistence, 153 restored preset records, 182 audio goldens,
32 native/WASM profiles and target DSP budgets pass. Hardware startup timing
and SPL boot with the larger directory extent remain unverified.
