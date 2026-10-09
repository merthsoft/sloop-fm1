# FM1 sample encoding foundation

Standalone platform-neutral .NET 10 library. No Android, Sloop.Core, protocol,
transport, hardware, or external package dependency. Source files and edits remain
owned by the caller; conversion never writes them. Keep original recordings and
persist selected gain/downmix, mapping and loops in the sampling document.

Run the dependency-free test executable:

```powershell
dotnet restore android/src/Sloop.SampleEncoding.Tests/Sloop.SampleEncoding.Tests.csproj --configfile android/src/Sloop.SampleEncoding.Tests/NuGet.Config
dotnet run --no-restore --project android/src/Sloop.SampleEncoding.Tests
```

In a sandbox that denies the user NuGet config, set APPDATA to a writable temporary
directory for the command process before restore. No package feeds are required.
The library also builds independently with `dotnet build` on its csproj.

## Integration boundary

Supply already decoded and edited/extracted slice PCM to `ConversionPipeline`.
`SourceZone` uses interleaved doubles in [-1,1], actual source rate/channel count,
explicit `ConversionOptions` and MIDI root/ranges. PCM16 import maps signed samples
to doubles by division by 32768, never by 32767. Input buffers must not be mutated
concurrently while conversion runs. The library copies intermediate PCM, and
`SlotBuilder` snapshots its caller PCM. Output arrays belong to the caller and can
be retained/exported; changing them does not mutate input recordings. Run DSP on
a worker; this is an offline implementation, not a real-time streaming engine.

`ConvertZone` returns derived PCM16 and clipping/cancellation diagnostics for
review. `SlotBuilder.Measure` reports exact fit before assembly; use converted
lengths, not source duration. `Build` fails on overflow. No fit operation silently
trims, drops zones, changes pitch, normalizes or compresses time. If the user chooses
such an edit, derive a new source slice and convert again. `ConvertedKit` reports
conversion diagnostics per input zone and the final artifact. `ZonePreview` is
decoded from the actual ADPCM bytes, in header/root order, with original input
indices. This previews encoding loss, not device filters/pitch/envelopes. Loop
metadata is in the header; `ImaAdpcm.Decode` can decode from an odd/even loop sample
using the saved state for repeated-loop audition.

Gain is a finite nonnegative linear multiplier and never implicit normalization.
Average-all, first-channel and selected-channel are explicit mono choices. A
cancellation frame has average channel magnitude > 0.01 and summed magnitude
< 10% of summed absolute channel values; this is an advisory threshold, not a
phase-correction edit. Clipping is counted after resampling, including values above
the maximum signed PCM16 value. Quantization multiplies by 32768, truncates toward
zero and saturates to [-32768,32767]. Input NaN/infinity/out-of-range values fail.

## Numerical and wire contracts

Resampling uses double-precision normalized Blackman-windowed sinc, 32 zero
crossings per side with wider support on downsampling, cutoff at the lower Nyquist
frequency, and constant edge extension. Output count is ceil(frames*22050/rate).
Same-rate conversion bypasses filtering. This is band-limited to finite FIR
accuracy, not an ideal infinite filter. Math.Sin/Cos and floating-point summation
can vary slightly across platforms; do not promise identical ADPCM bytes across
different resampling implementations. Golden wire comparisons deliberately use
identical PCM16 encoder input. Python's CLI uses moving-average/linear resampling
and peak-normalizes to 30000; those behaviors are deliberately absent here.

Loop coordinates are half-open slice-relative source frame boundaries. Both
boundaries map upward with the same rational ceil formula. A collapsed converted
loop fails explicitly. `ZoneInput` loop boundaries are in converted sample units;
wire loop end is endExclusive-1. Nonlooping zones write start=0, end=n-1, state=0/0,
looped=0. Loop state is captured immediately before encoding the loop-start sample;
the full stream still starts at predictor/index zero. Low nibble first, zero high
padding nibble for odd sample count; padding is never previewed as an extra sample.

All header values are explicitly little endian. Signed predictor/root use i16;
root is MIDI note*16 and 22050/44100 Q16 rate is exactly 32768. Up to 16 nonempty
zones, root/range 0..127. Explicit key ranges are inclusive. Omitted ranges split
between sorted roots with the Python midpoint rule; invalid empty splits fail.
Overlapping explicit ranges are allowed intentionally and firmware chooses the
first matching zone in sorted root order. Data remains in original input order;
stable sorted header entries retain offsets to their original data.

`DrumZone` uses the verified GM lane notes in web/EDITOR_PROTOCOL.md (36,35,38,39,
42,46,44,37,40,43,48,49,51,70,63,56), each as root/low/high. `ChopZone` uses an
explicit caller MIDI note, never guessed white-key numbering. Instrument mapping
uses supplied roots and optional inclusive key ranges.

Names uppercase invariantly, expand ASCII-producing Unicode ligatures/sharp-s,
filter to printable ASCII, truncate to eight bytes and zero-pad. Unsupported
characters are discarded, not transliterated. The golden name exercises accents,
CJK and sharp-s. Unicode casing tables can differ by Python/.NET version; callers
requiring absolute name portability should supply printable ASCII names.

The slot is 81,920 bytes; header 480; data begins at 512; capacity 81,408 data bytes.
Each zone independently uses ceil(n/2) bytes, then data is concatenated without
extra alignment. CRC-32/ISO-HDLC (zlib) covers only encoded data. Fit reports sum of
real samples, PCM16 bytes, encoded bytes, remaining bytes (negative on overflow),
duration at 22050 Hz and artifact bytes including the 512-byte prefix. Summed
duration excludes odd padding. Capacity has no universal exact duration when
multiple odd zones consume padding. Image is the occupied prefix through the data
end, with zeroed bytes 480..511; it is not a padded full-slot flash image.

Only single-slot artifacts are implemented. USR3+4 is deferred: existence in the
protocol is insufficient without captured editor/firmware routing and golden
fixtures for cross-slot mapping. Do not concatenate these artifacts and call that
a verified combined kit. Hardware audition/readback remains an integration check.

## Fixtures and attribution

`Sloop.SampleEncoding.Tests/generate_fixtures.py` imports the existing
`tools/sampleio.py`; run it with Python to regenerate committed fixtures. Provenance
records Python version and oracle SHA-256. Fixtures cover silence, impulse,
alternating extremes, odd samples, sorted multiple zones, 16 drum zones, Unicode
name, loop state and inclusive bounds, exact capacity (including per-zone odd
rounding). Generator also verifies Python rejects overflow. Tests compare every
header/data byte and firmware-formula decoded sample and test overflow/validation,
CRC standard vector, pipeline loop mapping, no source mutation, gain/downmix and
resampling pass/stop bands. They are a console runner with nonzero exit on failure.

Format/encoder adaptation is GPL-3.0-only, from tools/sampleio.py and
firmware/src/eng_sample.c, Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton
Instruments. Files carry SPDX attribution and this project includes LICENSE copied
from the repository. Preserve notices and comply with GPL distribution/source
requirements when integrating. No third-party DSP code or DSP dependency is used.
Read tools/fm1_sample_upload.py and web/EDITOR_PROTOCOL.md alongside these sources
for future integration. Do not add transport or firmware write behavior here.
