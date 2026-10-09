# Generic MIDI connection mode

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Implemented checkpoint: 2026-10-07.

The connection menu offers SLOOP FM1 and generic MIDI separately. Generic mode opens the
Android-enumerated destination's first MIDI input port, without requiring an output port,
creating an EditorClient, or sending any SLOOP handshake, SysEx, polling or device mutation.
USB class-compliant instruments and interfaces are the primary target. Other already
registered Android MIDI destinations can appear; BLE discovery/pairing, network MIDI,
MIDI 2.0, destination multiport selection and device presets are not implemented. External controller input has a separate opt-in device/output-port selector in Perform (see below).

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

## External controller input — 2026-10-09

Perform has an External controller button, initially off. Choose an Android MIDI 1
byte-stream device and named output port, then choose literal keyboard or chord roots.
This receive-only adapter opens an Android output port because that is the device's
source of incoming bytes. It never uses the protocol receiver or opens a send port.
The entire current destination device is excluded, including generic destinations;
this intentionally excludes combined keyboard/synth devices used as the destination.
Separate devices can still loop through physical MIDI wiring or DAW routing; avoid
routing the destination back into the selected controller. BLE pairing/discovery is
outside this feature, but devices already registered with Android can be selected.

Destination session changes and backgrounding close
controller input; explicitly reconnect from Perform. Busy device operations/playback reset held notes and suppress incoming input until ready. Track/workspace changes, release,
settings menus and focus loss clear notes and queued callbacks. Selection is Activity
scoped and is not persisted or automatically reopened after rotation. No hardware
acceptance was performed for this addition. Android port-open/exclusivity, USB host
power, device removal and physical latency still require device acceptance.
