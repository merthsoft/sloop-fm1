# Audio engine and recording

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

## Separate paths

FM1 capture receives stereo USB master audio. Microphone capture is a different source.
Local preview plays slices and recordings through the explicitly selected output. The FM1
USB playback route is the intended destination when available, with sound emerging from its
speaker or connected headphones. Hardware sample audition triggers uploaded zones via MIDI.
The checked-out SLOOP firmware implements capture on EP4 IN and adaptive playback on
EP4 OUT, with a bounded rate-matched return ring and mixer integration. Host tests and
the firmware build pass; physical FM1/Android/Windows duplex validation remains pending.
See [USB-PLAYBACK.md](USB-PLAYBACK.md). MIDI-triggered FM6 audition already
generates sound inside the FM1 and does not need audio OUT.

Basic WAV preview now exposes actual Android outputs. An explicit device must be present,
accept the preference and match the active route; otherwise playback stops. Confirmation
begins muted and restarts the slice once confirmed. Removal or a changed explicit route
stops playback. System default follows Android routing. Explicit MediaPlayer routing requires
Android 9; Android 8 uses system output. FM1 playback remains physically unverified.
Software monitoring is not implemented. A future opt-in monitor must display measured latency;
direct FM1 monitoring avoids that extra path when playing the instrument.

## Capture contract

Implementation checkpoint: recording requests RECORD_AUDIO at the recording action,
enumerates microphone/USB inputs, verifies the active AudioRecord route and captures PCM16
through a 16-block bounded writer queue. Microphone requests 48 kHz mono; USB requests
44.1 kHz stereo. Actual stream format and source/processing mode are preserved. Unprocessed
capture is requested when supported; the labelled fallback may include processing.
Android-silenced capture stops. Takes stop at two minutes or on leaving the foreground;
recording keeps the screen awake and shows elapsed time/peak without software monitoring.

CaptureJournal retains written PCM and format/source metadata until WAV adoption. Restart
recovery retains complete written frames and labels interruption; incomplete trailing frames
and unwritten queue buffers are not recovered. This handles process exit, not every power-loss
or storage-failure case. Native buffer continuity and actual FM1 USB capture remain physical gates.

Capture settings request route, channels, and sample rate; the service reports actual values.
Preserve actual capture format in metadata rather than relabeling 48 kHz data as 44.1 kHz.
Keep capture raw enough for music: avoid speech processing where supported, and disclose
source limitations. Calibration/metronome placement uses captured frame timestamps.

State: Idle -> Preparing -> Recording -> Finalizing -> Complete, with Interrupted/Failed exits.
Stop is idempotent. Each completed frame belongs to exactly one take. Write recoverable PCM
chunks with a metadata journal; finalize a WAV/export file outside the capture thread.
A disk writer consumes a bounded ring. On overflow, stop the take with a clear discontinuity
instead of pretending audio is continuous. Never stall the capture callback on storage.

## Preview engine

`Sloop.SampleEncoding` supplies decoded PCM16 preview buffers from the actual FM1 ADPCM
artifact at 22,050 Hz, plus loop metadata and clipping/cancellation diagnostics. These buffers
are integrated into Android encoded-zone preview using temporary mono 22,050 Hz WAVs and
the same verified output routing as source preview. Conversion/decoding runs offline on
a worker and does not implement a streaming, multi-voice or native audio backend. Local
encoded-result preview represents codec loss; FM1 pitch, filters and envelopes still require
device validation. See [SAMPLING.md](SAMPLING.md) for the conversion contract and tests.

The current backend is MediaPlayer, with one preview at a time. Perform pads emit MIDI to
FM1 rather than playing local PCM voices. The following multi-voice/native backend is planned;
adopt C++/Oboe for low-latency local sample playback only when needed.
Preload bounded slice audio off the real-time thread. Voice commands carry stable voice ID,
source handle, range, gain, pitch, playback mode, and desired timestamp. Support one-shot,
gate, and later loop modes. Apply short ramps to avoid clicks. A limiter protects the local
mix from accidental clipping without changing saved source material.

A narrow C ABI is the C# interop boundary: opaque engine handles, fixed-size command/status
records, explicit ownership and destruction. Do not invoke managed callbacks per audio frame.
Native buffers remain valid until the audio thread acknowledges release. Device reopen drains
old handles safely and reports a discontinuity. Exact ABI is deferred until choosing a backend.

## Clock and performance

Capture frames, phone monotonic time, and FM1 musical time are different clocks. Store mapping
anchors and discontinuities; estimate drift over time. UI playhead interpolation is visual
only. Long phone/audio sequences need rate-aware scheduling; USB capture cannot be assumed
sample-synchronous with MIDI notes just because both use the same cable.
Request low-latency settings but report actual buffer size/sample rate and measured latency.
No universal latency promise. Avoid Bluetooth monitoring as a default for live pads.
See [Android high-performance audio](https://developer.android.com/ndk/guides/audio) and
[Oboe](https://github.com/google/oboe).

## Analysis and acceptance

Workers compute peak pyramids, RMS/meters, transient candidates, and conversion previews.
Meter snapshots are decimated; UI cannot request synchronous DSP work on every frame.
Validate sustained multi-pad playback during waveform scrolling, long USB capture while
editing, writer backpressure, route removal, and repeated start/stop. Report underruns,
overruns, drift, actual latency, and CPU cost on target phones before selecting buffer defaults.
