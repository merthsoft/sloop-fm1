# Sample editor, chopping and transfer

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

## Retained sample library and tap chopping — October 8, 2026

Browse retained samples lists valid owned WAV assets in the active session, with duration,
chop count and current-source marker. Reuse restores the WAV's saved edits, kit settings and
chop audio settings without copying or overwriting source files. Invalid/reparse-point assets
and sidecars are excluded; overflowing/unreadable sidecars are isolated per asset; opening validates ownership and settings before publishing the
new current pointer. The active-session browser remains available inside the sample library.

The sample library also indexes manifest-listed WAVs in named immutable session snapshots on
a worker, with search by sample or source-session name, source-session labels, duration and chop
count. Close cancels indexing; reuse offers Cancel. Indexing is bounded to 1,024 directory entries,
4,096 valid samples and 2 GiB of sample/sidecar hash reads, and reports truncation/skipped corruption.
Corrupt WAVs or sidecars are isolated per asset, so an unrelated damaged asset does not hide a
valid sibling. Staging directories and unarchived workspace generations are not indexed.

Cross-session reuse validates source hashes again, copies the unchanged WAV plus `.edits`,
`.kitsettings` and `.chopaudio` into private staging under the active sample directory, flushes and
validates the copies, then publishes sidecars before the WAV and durably replaces the current
sample pointer. Exact RIFF extent/chunks, ordered chop frames, mapping settings and bounded
per-chop audio metadata are checked. Unmanifested sidecars reject; generated `.fm1` images are
not copied. Source-frame coordinates remain associated with their original WAV bytes. Identical
WAV-plus-sidecar state reuses an existing validated copy; edited prior copies are preserved and
a new copy is created. Missing/changed source snapshots fail with the current sample retained.
Session-directory changes during awaits reject adoption; successful reuse clears prepared-kit,
proposal, tap-review and pending-export state. Originals and immutable snapshots are never edited.

`Sloop.SampleLibrary.Tests` passes 26 focused filesystem checks for exact copies/metadata,
duplicate handling, edited-copy preservation, corruption isolation, stale/missing sources,
archive boundaries and cancellation before/during copying. It source-links production files
to avoid shared build outputs. Run `dotnet run --project android/src/Sloop.SampleLibrary.Tests`.
Android compilation, installation and physical UI acceptance are left to combined integration.

Start fresh tap pass plays the trimmed source. Tap marker reads MediaPlayer's actual position,
maps milliseconds plus source offset to frames, and supports explicit 0..500 ms compensation.
Markers stay separate from saved chops, show on the waveform, and support Undo last tap,
Stop/review and Discard. Apply replaces boundaries in one existing undo step. Changed source,
revision or trim invalidates the review; both playback-window edges (including latency-clamped
start positions), duplicates and more than 15 boundaries reject.
Processed/encoded alternate playback cannot supply taps for the original source.

Sampling tools pass 70 focused checks and the combined APK builds with zero warnings/errors.
This follow-up has no new physical touch or latency measurements; compensation is manual.

## Chop-to-kit review — 2026-10-07

The selected chop now has source audition and **Map & prepare kit** directly below the
waveform. The Send page offers selected source/encoded audition and a collapsible source-order
assignment list with MIDI note names, roots and key ranges. Encoded chop audition resolves
the original input index, including drum lanes whose wire zones sort into another order.
Saved options reject invalid mappings/gain and root ranges before conversion; unchanged saves
retain the prepared result. Derived images use the existing durable atomic file writer.

A proposed USR1–4 target is visible and resets on source change. The KitUpload callback now
uses that proposal in an explicit replacement confirmation, with Other slot and Cancel.
Every existing backup, journal, stale-result and readback safeguard remains in the send path.
The 79 assignment checks are registered in the normal Workstation test runner.
No device stopped-state detection or automatic free-slot selection is claimed.

The standalone kit-plan runner passes 79 checks comparing all mapping plans against actual
encoded headers. Current combined build/test evidence and earlier Pixel sampling checks
are linked from VERIFICATION.md.

## Implementation checkpoint — 2026-10-07

Combined integration: zoom/pan/fit, exact chop-edge edits, transient preview/apply/discard
and revision-aware kit invalidation are connected to the app. Split/Undo/Play/Stop appear
directly below the waveform; zoom/navigation is expandable. The sample survives named
session adoption and restart; see [verification record](VERIFICATION.md).

File-backed PCM16 WAV import, fixed-size waveform peaks, half-open frame trim/markers,
manual and equal chopping (up to 16 slices), undo, atomic edit-file replacement, local
slice audition and WAV export are implemented. Source WAV is copied into internal storage;
current asset and edits restore on restart. Import is capped at 256 MB and worker-based.
Android microphone/USB capture now feeds recoverable PCM16 takes into the same editor;
user hardware acceptance is complete for now; no new route measurements were made. Undo history is currently in-memory.
The UI separates Chop and Send to FM1. The larger waveform accepts tap/drag cursor placement
without rebuilding the screen, alongside a fine slider and paired Split/Undo and Play/Stop.
Trim and equal chopping are expandable; slice-count selection survives rebuilds. Chops use a
four-column selection/audition pad bank with selected-chop export. Cursor dragging selects source
frames in the visible viewport. Zoom, pan, full-WAV, fit-trim and fit-selected-chop controls retain
the viewport across view rebuilds. The selected-chop section accepts exact inclusive start and
exclusive end frames, one-frame cursor nudges, cursor-to-edge moves and removal of an internal
end boundary to merge with the next chop. Both edge changes form one undo step and keep
neighboring slices nonempty.
Send shows a capacity meter, mapping, name/root, preparation and encoded audition. Transfer
shows backup, upload percentage, readback verification and retained recovery status; stale
sample/mapping/name/root changes require preparing again. Input fields retain drafts during
view rebuilds. USB slot replacement still requires stopping the FM1 song manually.
Gain, downmix/mapping/loop controls, FM1 conversion, decoded preview and backed-up upload
are integrated. Zoom and transient proposals are implemented. Tap-along detection, pitch UI and a full retained-assets browser remain planned. Preview uses
Android MediaPlayer with temporary WAV extraction and stops when the Activity backgrounds;
this is a replaceable basic audition backend, not the planned multi-voice instrument engine.

The independently buildable .NET 10 [Sloop.SampleEncoding library](../src/Sloop.SampleEncoding/README.md)
now implements the offline PCM-to-FM1 foundation: explicit downmix/gain, band-limited
22,050 Hz conversion, PCM16 quantization, IMA ADPCM, single-slot kit assembly, mapping,
loops, CRC, exact fit reporting and decoded encoded-result preview data. It has no Android,
Core or Protocol dependency. Workstation now connects it to the sampling UI and audio service.
The caller retains source recordings and edit documents; conversion creates derived buffers.
Device upload/readback and restoration are integrated with CRC-checked backups. Combined
Combined USR3+4 remains future work; user hardware acceptance is complete for now.

## Document and workflow

A source asset is immutable. `SampleDocument` stores its source reference, trim and ordered
internal boundaries using half-open [start,end) source-frame ranges. Its monotonic revision and
undo stack are process-local; the source path, trim and boundaries restore from durable files.
Kit mapping, gain, mono choice, root and loop settings are persisted separately by the sample
workspace. Per-chop gain/tuning and root overrides now persist in a separate `.chopaudio`
sidecar keyed to exact source ranges. Stable slice IDs and ordered processing chains remain planned.
Cursor movement and viewport navigation do not edit the document. Successful edits persist
without re-encoding; the user explicitly prepares a fresh kit after changing edits.

Capture/import -> trim -> chop -> audition -> map -> fit -> encode -> upload -> verify.

Current mapping choices: Chops assigns each slice one trigger key; DrumLanes assigns FM1 drum
notes; Instrument creates key ranges for pitched playing through the firmware Sample engine
on a synth track. Firmware repitches relative to zone root by changing playback speed, so
duration changes with pitch (one octave up doubles speed). Root metadata can be entered manually
or applied from a reviewed offline pitch estimate. Per-chop tuning is baked into derived audio;
root metadata should describe the resulting pitch. Duration-preserving time stretching remains planned.
Import and restore scan the file on a worker into 512 waveform peak buckets. Zoomed views scan
only the visible source range on a cancellable worker with the same bounded bucket storage.
The waveform and transient analyzers do not load the whole recording into RAM; conversion
extracts derived slice buffers separately. Import currently supports mono/stereo PCM16 WAV
at 8–192 kHz, capped at 256 MB. Other audio formats remain unsupported.

## Chopping behavior

Manual splits, equal parts and transient proposals use the same ordered boundary model, capped
at 15 internal boundaries (16 slices). Internal boundaries can move only between their immediate
neighbors, or be removed to merge adjacent slices. Exact selected-slice range edits change both
edges atomically; first/last edges change the trim. Each slice and neighbor retains at least one
source frame. Invalid requests leave edits and undo history unchanged.

Transient analysis proposes boundaries without changing manual edits. Sensitivity and minimum
spacing control the analysis; orange lines preview candidates over existing yellow boundaries.
Apply explicitly replaces the boundaries in one undo step; Discard keeps the current chops.
Preview is visual; audition uses the resulting chop pads after Apply. Editing or undoing the
document invalidates its outstanding proposal and prepared kit.

Tap-along markers, tempo-grid chopping, optional bounded zero-crossing adjustment, advanced
overlap/gap mapping and stable slice identity across reorder remain planned. A future tap-along
implementation must use playback frame position with output-latency compensation.

## Conversion pipeline

Decode -> source edits -> selected slice extraction -> explicit mono downmix -> band-limited
resample to 22,050 Hz -> quantize -> firmware-compatible IMA ADPCM -> slot layout/header/CRC.
Gain/normalization is an explicit saved option; do not silently copy the CLI's peak-normalizing
helper behavior into all app uploads. Detect clipping and stereo cancellation at downmix preview.
Decode the encoded result for FM1-format preview; do not audition pre-encode PCM and label it
converted. Device pitch/filters may still differ from local preview.

Implemented entry point: `ConversionPipeline` accepts already decoded, edited and extracted
source slices, with interleaved normalized doubles, actual source rate/channel count, explicit
conversion options and mapping. Average-all, first-channel and selected-channel downmix are
supported; gain is a nonnegative linear multiplier. No normalization is implemented.
Reports count cancellation frames and post-resampling clipped samples. Quantization scales
by 32768, truncates toward zero and saturates to signed PCM16. Invalid PCM/settings fail.

The resampler uses normalized Blackman-windowed sinc with 32 zero crossings per side,
scaled support for downsampling and constant edge extension. Converted length is
ceil(sourceFrames * 22050 / sourceRate); same-rate conversion bypasses filtering. This
intentionally differs from the Python CLI's moving-average/linear resampler and peak
normalization. Golden wire comparisons use identical PCM16 encoder input; floating-point
resampling is not guaranteed byte-identical across platforms or implementations.

Source loops use half-open slice-relative frame boundaries, mapped upward using the same
rational ceil formula. Collapsed converted loops fail. Wire loop ends are inclusive, and
predictor/index are captured immediately before the loop-start sample. Every stream starts
at predictor/index zero, packs low nibble first and zero-pads an odd final high nibble.

Source of truth: [sampleio.py](../../tools/sampleio.py),
[eng_sample.c](../../firmware/src/eng_sample.c), and [protocol](../../web/EDITOR_PROTOCOL.md).
Current slot is 81,920 bytes, data starts at 512, usable data 81,408 bytes, and header is 480
bytes. Up to 16 zones carry offsets, lengths, loop points, fixed-point rate/root, initial
ADPCM state, and key ranges. CRC covers the encoded data. Match nibble ordering and initial
predictor/index exactly; this is not an arbitrary WAV IMA block format.

Single-slot capacity is computed from encoded slices (including per-slice odd-sample rounding),
not a rounded duration. Combined USR3+4 kit layout must match the existing editor/firmware
mapping; defer implementation until that exact mapping is captured in conversion fixtures.

## Mapping and fit

Drum kit maps slices to device lane notes; chop set maps slices to discrete keys; instrument
maps zones to root notes/key ranges. Offer a named preset for each mapping and show actual
notes. Do not use guessed white-key numbering as a wire note map.
The library provides `DrumZone` using verified GM lane notes, `ChopZone` with an explicit
MIDI note, and instrument zones with supplied roots and optional inclusive key ranges.
Header zones sort stably by root; data retains input order and offsets. Omitted key ranges
use the tooling's midpoint split. Explicit overlapping ranges retain first-match semantics.
Root is MIDI note * 16; the fixed 22,050 Hz Q16 rate field is 32768. Android exposes Chops,
DrumLanes and Instrument mappings with root/downmix/gain and full-slice loop controls.
Reusable named user presets remain planned.

`SlotBuilder.Measure` reports real sample count/duration, PCM16 bytes, ADPCM bytes and signed
remaining capacity; assembly rejects overflow. Data bytes are sum(ceil(zoneSamples / 2)).
The exported image is the occupied prefix through the data end, including zeroed header/data
gap, rather than a padded 81,920-byte flash image. No automatic fit edits are implemented.
Names uppercase, filter to printable ASCII and truncate/zero-pad to eight bytes; ASCII-producing
ligatures/sharp-s are expanded. Unicode casing can vary by runtime; portable names use ASCII.
Fit shows the before/after byte count and audible effect. Offer dropping slices, shortening
tails, or deliberate time compression. Lowering playback pitch is not a storage optimization.
Keep the long original and independent derived result. Export slices/kit metadata separately.

## Upload operation

1. Encode and validate entirely before touching the device.
2. Explicitly choose USR1–4; read and CRC-check its backup, including an empty destination.
   Flush backup, replacement and pending journal to phone storage before destructive writes.
3. Require released app performance/preview; the confirmation asks the user to stop FM1.
   The app does not yet query or enforce a firmware stopped-state command.
4. SMP_BEGIN invalidates the header. Write ordered blocks of up to 256 raw bytes at aligned
   offsets from 512; pack7 for SysEx and verify echoed offset/status.
5. SMP_END sends the header; firmware checks data CRC and commits the header last.
6. Read back through BK_LIST/BK_GET, validate CRC and compare header/audio data. The reserved
   32-byte flash gap is not musical data and may differ from the zeroed exported image.
7. Retain both images and update the transfer journal only after verified completion.

After BEGIN, cancel means stop sending and leave an incomplete/empty slot. Make this explicit.
Uncertain chunk completion triggers recovery/restart, not a blind rewrite at a sector boundary.
Device removal cannot automatically restore the old slot. A restored slot is another deliberate
upload using its backup. Background navigation must not orphan an active operation.

## Acceptance

The standalone `Sloop.SampleEncoding.Tests` runner passes 102 assertions across nine committed
Python-generated golden fixtures: silence, impulse, alternating full scale, odd length,
multiple zones, 16 drum zones, Unicode names, loops and exact capacity including odd rounding.
Headers/data/CRC match the existing tooling for identical PCM16 encoder input; decoded preview
samples match a firmware-formula oracle. The fixture generator also checks Python overflow
rejection. Tests cover validation/overflow, loop-state restoration and boundary mapping,
source immutability, gain/downmix, standard CRC and resampling DC/pass/stop bands.
Fixture provenance, regeneration/build commands and GPL-3.0-only attribution are in the
[library README](../src/Sloop.SampleEncoding/README.md).

Converted-result playback and persistent settings/artifacts are integrated and exercised on
Pixel with a synthetic WAV. Workstation fixtures cover conversion/mapping, CRC backups,
aligned writes, reserved gaps and restoration decoding. First/middle/final physical transfer
failures, stopped-state verification and audible FM1 round-trip remain acceptance gates.
Original asset and edit document must survive every failure.

## Sampling tools implementation — 2026-10-07

`SampleViewport` maps touch/slider fractions to half-open source frames, anchors zoom at the
cursor and clamps navigation to the immutable source. Zoomed envelopes are read on a worker
with 512 peak buckets; detached views cancel their reads. No PCM array grows with WAV length.
Buttons provide practical touch navigation; pinch zoom and direct boundary dragging are not
implemented. Boundary changes use the selected slice controls, avoiding accidental edits while
positioning the cursor.

`SamplingTools.Detect` streams a 2 ms maximum-channel envelope, detects rising energy relative
to an adaptive baseline, and retains at most 15 strongest spaced candidates. It avoids stereo
cancellation by analyzing channel magnitudes independently. Sensitivity and 20–320 ms minimum
spacing are exposed. Onsets resolve to the start of an analysis window (approximately 2 ms);
manual edits remain exact to a single source frame. This is a practical onset heuristic, not
musical beat or pitch detection. Analysis supports cancellation and has fixed memory use.

Orange proposal lines overlay existing yellow boundaries. Preview is visual and does not change
edits or converted audio. Explicit Apply replaces all chop boundaries in one undo step; Discard
keeps current boundaries. Proposals are tied to document identity and monotonic revision, so an
edit or undo invalidates them. Conversion keys include that revision, and conversion clears the
previous result before rebuilding. Every edit, including undo, therefore requires fresh kit
preparation before encoded audition or transfer. Existing output selection and confirmation,
recording recovery, WAV export, conversion protocols and two-step slot confirmation are retained.

The standalone `Sloop.SamplingTools.Tests` .NET 10 runner passes 32 checks covering boundary
moves/removal, atomic slice-edge edits and undo, invalid edits, proposals on silence and opposite
stereo impulses, trim and spacing, cancellation, zoom peak extraction and cursor/navigation,
persistence, exact WAV extraction and byte-for-byte source preservation. The existing workstation
runner passes 61 checks. A full Android build succeeded before the final conversion-key change;
the final UI/service code compiled successfully, then packaging was stopped at the originating
chat's request to avoid overlapping shared builds. That chat owns the combined build/deployment.
No phone install or physical touch/audio/USB acceptance was performed by this slice.

Run the standalone sampling checks with:

```powershell
dotnet run --project android/src/Sloop.SamplingTools.Tests/Sloop.SamplingTools.Tests.csproj
```

See [verification record](VERIFICATION.md) for changed files and solution-registration
requirements.

## Pitch/root and chop audio — 2026-10-08

Selected-chop controls now estimate offline raw-source pitch (Hz, nearest MIDI root and
normalized autocorrelation confidence), offer an explicit reviewed Apply, and accept a manual
root override. Detection uses the strongest channel, bounded 2,048-frame analysis and a
50–1,200 Hz lag search. Silence, short material and weak periodicity return no estimate.
Confidence expresses periodic similarity, not a calibrated probability; polyphonic material,
strong harmonics and aliased high-frequency content may give wrong roots. Manual review is
required. Estimation does not change audio or mapping until Apply.

Chop gain (0–8) multiplies kit gain; tuning (−24…24 semitones) changes playback speed/duration
in the derived band-limited conversion. The effective source rate is rounded to an integer;
fit and conversion use the same rate. The encoded rate remains 32,768 Q16 and root remains
MIDI note × 16. Root overrides set actual zone roots/triggers; Instrument key ranges follow
sorted midpoint splits, while Chops/DrumLanes use single trigger keys. Duplicate roots reject
preparation. Tuning is baked into audio, separate from root metadata: after tuning a pitched
instrument, set root to the resulting pitch. Detection reports the untuned source.

Selected-chop/pad/assignment audition uses conversion plus ADPCM decoding with saved settings;
whole-source Play and original WAV export remain raw. Encoded audition uses prepared data.
Settings changes invalidate preparation; source WAVs and transfer safeguards are retained.
Per-chop settings persist in a validated `.chopaudio` JSON sidecar keyed to exact source ranges.
Boundary changes do not carry settings onto new ranges; Undo restores matching ranges until a
later settings save prunes inactive records. Stable slice IDs and durable processing undo remain
future work. Corrupt settings report an error and block conversion/processed preview until an
explicit settings save repairs them.

The dedicated SamplePitch runner passes 27 new focused checks plus 79 existing mapping checks.
Combined build succeeds with zero warnings/errors and the APK is installed. Portable sessions
include `.chopaudio`; import, startup and staged adoption validate size, entries and source
frame bounds before publication. Older sessions use default chop settings.

