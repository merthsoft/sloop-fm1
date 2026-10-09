# FM1 USB playback implementation

Implemented in `src/usb.c`, `src/audio.c`, and `hal/fm1_usb.h`. No hardware
was flashed or tested. Android, Windows, and the physical controller remain
acceptance gates.

## USB resources and compatibility

UAC1 PCM16 stereo, 44,100 Hz: IF2 capture stays at EP4 IN (`0x84`);
new IF3 playback uses adaptive EP4 OUT (`0x04`), idle/active alternate
settings, 184-byte maximum packets at 1 ms. Terminals 3 (USB streaming)
and 4 (speaker) describe playback separately from capture terminals 1/2.
CDC shifts to IF4/5; MIDI IF1 and its endpoint addresses are unchanged.
Configuration lengths are 249 bytes plain and 323 with CDC. Device versions
are 3.20/3.21; the loader remains MIDI-only at its previous version/PID.
CDC compiled in but hidden produces the exact same descriptors as no CDC.
The existing shared audio/MIDI AC collection and IAD scheme is preserved.

EP4 has dedicated TADR/RADR registers at `0x11838`/`0x1183c`, independent
TX/RX CSR and MAXP registers. The EP1..3 address helper must **not** be used
for EP4: its arithmetic would alias EP4 CNT. Playback reserves its own RX
DMA buffer and configures RX ISO/flush separately, without flushing capture.
RX MAXP is 184; RX interrupt mask adds bit 4. There is no new endpoint number
or dynamic MUSB FIFO allocator. Capture still reserves 184-byte TX packets;
the software register/DMA model verifies the direction separation. The HAL
does not expose FIFO size or silicon duplex arbitration; actual simultaneous
EP4 IN/OUT operation and MAXP behavior must be verified on FM1. Register/model
checks are not physical FIFO validation.

Nominal duplex PCM traffic is 352,800 bytes/s, maximum descriptor payload
368,000 bytes/s before USB overhead, plus existing MIDI/CDC. Both audio
directions run in the existing 2 kHz TIMER5 EP4 service, including while it
nests inside an audio render. Playback remains serviced when capture is idle.
The extra idle service reads RX status; actual SIE/ISR time needs measurement.

Endpoint frequency controls accept only 44,100 Hz and a complete three-byte
SET_CUR data stage. Unsupported rates, malformed lengths/selectors/indices,
and unsupported alternate settings stall. Reset/unconfigure stops both
streams; suspend suppresses incoming return, fades output, and discards stale
queued samples. Stop and restart use a published epoch; only the audio
consumer writes the read index, including across unsigned index wrap.

## Audio and clock matching

A 1,024-frame stereo ring uses 4,096 bytes. DMA reserves 1,028 bytes so even
a malformed full-speed ISO payload fits before software validates RXCOUNT.
Malformed packets and packets that cannot fit are discarded whole; counters
track packets/frames, malformed input, overruns, underruns, hardware errors,
starts, fill extrema, and current Q16 rate (`up` in the ELF symbols).

The adaptive endpoint uses a stereo linear interpolator. Its nominal Q16
step is 65,510 (44,100/44,117.6), with filtered occupancy correction bounded
to approximately +/-1%. It can consume zero, one, or two input frames per
codec frame; it does not periodically delete a sample or drain at a fixed
nominal rate. This avoids an extra feedback endpoint and its host/controller
compatibility questions, consistent with the adaptive sink synchronization
type in the [UAC1 specification](https://www.usb.org/sites/default/files/audio10.pdf).
Linear interpolation is a bounded initial implementation, not a high-order
bandlimited converter; measure high-frequency fidelity before release.

At the checked-in 256-frame half size, priming needs 384 frames (~8.7 ms).
The ring is bounded to ~23.2 ms; codec DMA adds further latency. AS delay 15
USB frames is an estimated ring/codec delay, not a measured end-to-end promise.
Start, stop, and starvation use 256-frame (~5.8 ms) ramps. On starvation the
last interpolated frame fades to zero, then playback re-primes. Discontinuities
outside the +/-1% correction envelope are counted, not hidden indefinitely.

Return is a direct monitor after instrument effects and the capture tap, at
default -6 dB gain, with a ramped physical master-knob gain. Saturating addition
protects the codec bus. A silent return leaves synth samples bit-exact. Both
MASTER and FULL capture exclude the return; there is no combined capture mode.
The ISR allocates nothing and neither blocks nor accesses storage. Codec
scaling now uses multiplication instead of signed left-shift, preserving the
intended output values without undefined behavior for negative samples.

## Validation and budgets

Run `CC="<host compiler>" sh tests/run_usb_playback.sh`. Tests include existing
capture regressions, all three CDC variants and byte comparison, loader
descriptors/configuration, EP0 frequency and alternate controls, stereo/gain,
clipping, ramps, underrun recovery, malformed/full ring behavior, nested
playback-only service, suspend/reset, index wrap, and the actual audio_block
capture ordering. Two-minute duplex simulations at 44,117.6, 43,900 and 44,300
codec Hz consume real 256-frame render bursts alongside capture and MIDI.
All pass; nominal playback fill is 235..555 frames, opposite drift runs
264..643 and 146..546, without playback or capture under/overruns. Tests are
software models, not Android USB-driver or hardware tests.

The JieLi target build passes link, RAM, flash, MMIO, and RAM-code checks:
566,284-byte image, 88,180/98,304 bytes data+BSS (10,124 free), and
334,560/344,064-byte pool (9,504 free). A baseline using Git's USB/audio files
with the same remaining checkout is 563,840 bytes / 82,916 bytes RAM: playback
adds 2,444 bytes image and 5,264 bytes RAM, with no additional pool allocation.
Review artifacts are in `build/usb-playback/`; the baseline is in
`build/usb-playback-baseline/`, host logs in `build/usb-playback-tests/`.

Compiled `uac_play_mix` is 201 instructions, with no function calls. The master
interpolation divide is once per block. Idle playback exits before the sample
loop. The existing instruction-budget script reports audio ISR weighted cost
281 versus the comparable baseline's 268 (+4.9%); its checked-in budget of 174
already fails on that baseline. This measure excludes called playback DSP and
is **not** a CPU percentage. The new DSP and expanded nested service must be
included in a future shared budget update and measured using audio_max_us,
felucca_dbg late/max_us, and t5_nested_ticks under maximum synth/FX load.
Do not treat the stale shared budget or the link success as performance signoff.
Undefined-behavior trap tests pass. AddressSanitizer was attempted but its
runtime is unavailable in the bundled host compiler, so ASan is unverified.

## Remaining integration and physical acceptance

Negotiated gain/mute and diagnostic snapshots are implemented in
`src/editor_usb_playback.c`, `src/usb.c`, `EditorClient.UsbPlayback.cs`, and the
Android USB playback service/UI. Command 75 is reserved exclusively for this
feature, with INFO protocol 12; schema/capability query is suboperation 0.
Combined capture remains unimplemented; both capture modes exclude return.

Before shipping, inspect the installed descriptors; verify EP4 duplex FIFO/DMA
behavior, Windows/Android enumeration and class requests, speaker/headphone
output and mute/master behavior, sample-rate conversion, repeated starts,
unplug/suspend/resume, and MIDI plus long capture/playback under peak firmware
load. Measure CPU, latency, underrun/overrun counts and resampler fidelity on
FM1. Ensure instrument capture never contains phone return and explicit Android
routing stops on removal. Validate optional CDC separately. No physical result
is claimed by this implementation.

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
