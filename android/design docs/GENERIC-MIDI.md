# Generic MIDI connection mode

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Implemented checkpoint: 2026-10-07.

The connection menu offers SLOOP FM1 and generic MIDI separately. Generic mode opens the
Android-enumerated destination's first MIDI input port, without requiring an output port,
creating an EditorClient, or sending any SLOOP handshake, SysEx, polling or device mutation.
USB class-compliant instruments and interfaces are the primary target. Other already
registered Android MIDI destinations can appear; BLE discovery/pairing, network MIDI,
controller input, MIDI 2.0, multiport selection and device presets are not implemented.

Perform's chords, keys, scale grid, pads, ribbon, strums, arps and repeat send standard MIDI
note-on/off. An on-screen channel selector supports channels 1–16. Oct−/Oct+ changes the app
base octave; generic mode never follows FM1 hardware octave. Sequence offers per-track output
channel mapping during playback without rewriting stored notes (defaults 1, 2, 3 and 10).
Backgrounding, navigating away, releasing and disconnecting clean up app-owned notes.

FM6 RAM exchange, native pattern exchange, slot transfer and destination-transform checking
remain SLOOP-only and are disabled in generic mode. Local sound/sample editing still works,
but there is no generic patch programmer, sampler upload or automatic audio-return route.
The connected external instrument produces the sound; this mode is output/controller only.

Validation: Android builds; shared tests cover standard notes, channel 16, ownership and
timed cancellation. Phone menu/control checks are separate from live instrument acceptance.
No non-FM1 instrument is connected for an end-to-end generic MIDI acceptance test yet.
