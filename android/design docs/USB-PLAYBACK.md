# Phone playback through the FM1

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Status: app route selection, firmware playback and negotiated gain/mute/diagnostics are
implemented; focused host tests and target build pass. Earlier hardware validation is
accepted by the user; new controls still need physical audio measurements.

## Required behavior and current source

The phone is USB host. FM1 exposes capture, playback and MIDI together. App previews,
recordings and app-rendered instruments can play through the FM1 speaker or attached
headphones, alongside FM1 synthesis. MIDI-triggered FM6 sounds already use the instrument
output and do not need phone-rendered PCM.

The checked-out [usb.c](../../firmware/src/usb.c) exposes UAC1 PCM16 stereo 44.1 kHz
capture through EP4 IN (0x84) and adaptive playback through EP4 OUT (0x04).
[audio.c](../../firmware/src/audio.c) mixes host PCM after the instrument capture tap.
This describes this source/build, not every FM1 firmware. Inspect the physical instrument's
descriptors before identifying its installed firmware behavior.

## Implemented firmware path

Playback uses UAC1 PCM16 stereo 44.1 kHz, IF3 idle/active alternate settings and an
adaptive OUT endpoint linked to the speaker terminal. Capture stays at IF2, MIDI endpoint
addresses are unchanged, and CDC moves to IF4/5. Plain/composite descriptors, requests,
lengths and device versions (3.20/3.21) are updated; the loader remains MIDI-only.
EP4 has separate TX/RX DMA registers, checked in the host model. Actual duplex FIFO/DMA
arbitration and MAXP behavior require physical FM1 validation.

The nested 2 kHz USB service receives complete frames into a 1,024-frame bounded ring.
The audio ISR blends a direct monitor return after instrument effects and capture, at
default -6 dB gain, following the physical master knob with saturating addition. A silent
return preserves synth samples. There is no allocation, blocking or storage in the ISR.
Priming covers a full render plus jitter reserve; start, stop and underrun use 256-frame
ramps. Counters track malformed packets, hardware errors, over/underruns and ring fill.

Existing capture comments describe an audio clock near 44,117.6 Hz, not exactly 44,100.
Playback uses a stereo linear resampler with filtered occupancy correction bounded to
approximately +/-1%, rather than fixed draining. Adaptive OUT requires no feedback endpoint.
Two-minute host simulations at 44,117.6, 43,900 and 44,300 codec Hz pass with simultaneous
capture and MIDI, including 256-frame render bursts. Target CPU, latency and resampler
fidelity remain measurement gates.

Negotiated return gain/mute and diagnostics now use editor command 75 on INFO
protocol 12. Query suboperation 0 returns schema 1 and feature flags; older INFO
versions are not probed. Gain Q12 0..4096 scales the existing -6 dB monitor;
mute keeps the stored gain and fades the effective level to zero in 256 frames.
The ring keeps consuming while muted. Capture remains instrument-only.
The Android service negotiates physical connections and exposes return controls and
explicit diagnostic read/reset in the Sample workspace. These controls do not
identify or confirm an Android audio route. Phone validation remains pending.

The target build adds 2,444 bytes of image and 5,264 bytes of RAM versus
the comparable validation-build baseline, leaving 10,124 bytes of data/BSS RAM in that build.
The shared runner includes focused playback tests and static diagnostics include uac_play_mix.
Baseline and playback audio ISR budgets still fail; helper/FM6 entries have no accepted budgets.
Budgets were not raised and physical performance signoff remains open. See the
[implementation note](../../docs/firmware/USB-PLAYBACK-IMPLEMENTATION.md) for resource accounting,
reproducible checks, review artifacts and limitations.

## Capture bus and feedback

Both MASTER and FULL USB capture exclude phone return, preserving an instrument-only bus;
actual audio-block host tests verify this ordering. A combined mix option is not implemented.
Any future combined option must be explicit and labelled.
Monitoring FM1 capture back through the FM1 adds latency and can create feedback when
combined capture is selected. Validate bus placement in tests; direct hardware monitoring
remains available.

## Android adapter

SampleWorkspace lists actual AudioDeviceInfo outputs. The Sample workspace has an output
chooser with system default as an alternative. Explicit selection requests a preferred
MediaPlayer device and checks RoutedDevice while active. Confirmation is muted; successful
confirmation restarts the slice audibly. Failed confirmation or removal/route change stops
playback. Explicit routing requires Android 9+; Android 8 uses system output.

Show device names/types; do not identify an arbitrary USB headset as FM1. A later verified
association can remember the instrument. Android device IDs are transient; selection is
currently process-local. Neither simulator tests nor phone-speaker tests establish FM1
duplex support.

The same adapter now auditions decoded FM1 ADPCM zones as well as source WAVs. Perform
uses MIDI-generated FM1 sounds; a local PCM instrument/native FM6 renderer is not implemented.

## Validation

Completed: focused host descriptor/control, ring/ramp, malformed packet, reset/suspend,
index-wrap, capture-bus, duplex drift and MIDI tests; all three CDC variants, identical
hidden-CDC/plain descriptors and unchanged loader behavior; target firmware/loader build
and undefined-behavior trap checks. AddressSanitizer runtime is unavailable. No hardware
was flashed. The remaining physical and integration acceptance checklist follows.

- Physical descriptor/control checks: streams, alternate settings, EP4 FIFO/DMA duplex,
  CDC variants and reset/suspend/unplug behavior.
- Sustained hardware clock/load checks: ring fill, under/overruns, audible ramps,
  CPU/latency/fidelity and playback without clicks or growing latency.
- Phone WAV preview through actual FM1 speaker and headphones, including gain, source-rate
  conversion, and simultaneous MIDI/capture/playback under firmware load.
- Instrument-only capture excludes return on hardware; combined capture remains future work.
- Explicit FM1 output stops on disconnect instead of playing through the phone speaker.
  Windows provides a second host check; other phones require measurement.

Reference: [Android USB audio](https://source.android.com/docs/core/audio/usb).

## Negotiated command 75, schema 1 (host validated)

All payload bytes are 7-bit. Requests: `[0]` capabilities, `[1]` control read,
`[1,gainLo,gainHi,mute]` control write, `[2]` diagnostic snapshot, `[3]` counter
baseline reset plus snapshot. Gain is unsigned14 Q12 0..4096 (4096 means the
existing -6 dB monitor); mute is 0/1. Gain/mute publish together in one 32-bit
word. Audio applies a fixed 256-frame ramp with no per-frame divide, allocation,
blocking or storage. Physical master ramp and stream/starvation ramps multiply
this level; gain changes while muted persist for the next unmute. Muting does
not stop ring consumption. Saturation counts clipped frames once per stereo frame.

Replies start `[sub,status]`: status 0 success, 1 malformed, 2 unavailable.
Capabilities append `[schema=1,flags,maxLo,maxHi,rampLo,rampHi]`; flags are
1 gain, 2 mute, 4 diagnostics, 8 baseline reset. UAC-disabled builds advertise
flags 0 and return unavailable to controls/diagnostics. Control replies append
`[gainLo,gainHi,mute]`. Diagnostic replies append twelve unsigned32 counters,
each five LSB-first 7-bit bytes (last byte <=15), then host-selected playback
alternate (0/1). Fields: packets, frames, malformed packets, overruns, underruns,
hardware errors, starts, clipped frames, lifetime fill minimum, lifetime fill
maximum, current fill, resampler Q16 step. Alternate selection is not proof of
successful audible routing. Fill observations are bounded to the 1024-frame ring.
Snapshots are individual observations, not an atomic multi-counter instant.
Counters wrap modulo 2^32. Reset advances the first eight counter baselines and
returns zero for them; it preserves lifetime extrema, stream indices, DSP and
control state. Neither reset nor reads stop audio. No combined capture capability
is advertised.

Focused host evidence for this extension: all CDC variants and loader pass
`tests/run_usb_playback.sh` using the existing bundled Zig host compiler; new
cases exercise negotiated capabilities, malformed requests, mute/unmute ramps,
gain while muted, clipping counters, snapshots and reset isolation. Dedicated
wire tests pass with `dotnet run --project
android/src/Sloop.Protocol.UsbPlayback.Tests/Sloop.Protocol.UsbPlayback.Tests.csproj`.
They cover unsupported old INFO without a request, UAC-disabled capabilities,
control wire data, invalid gain/state, full uint32 diagnostics, reset and
malformed/overflow counters. The protocol library builds with zero warnings.
These host results do not resolve the existing CPU budget failure or replace
physical FM1/Android/Windows, latency, USB duplex and peak-load measurements.
