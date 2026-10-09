# Android platform services

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

MIDI/octave checkpoint (2026-10-07): Generic MIDI mode opens only the destination input port and skips the SLOOP handshake/polling. Perform has direct octave/channel controls; protocol-10 SLOOP links poll hardware octave every 100 ms outside device operations. Octave changes preserve latched chord pitches until the next chord press; touch views remain stable.

UI refinement checkpoint (2026-10-07): WorkspaceStyle supplies consistent rounded buttons, selected states, input/slider colors, paired actions and expandable sections. The shared connection menu replaces stacked connection controls; Sample and Library hide the irrelevant track strip. Global disabled transport placeholders were removed.

Status: C# Android adapters and five workspaces are implemented and integrated.
MidiManager/AndroidMidiTransport, process-scoped Fm1Connection, SampleWorkspace and
EditingWorkspace supply live services. Native views consume Workstation/domain libraries.
Pixel installation, editing, recording recovery and offline performance flows are exercised;
the user considers hardware acceptance complete for now. See [VERIFICATION.md](VERIFICATION.md)
for build and test evidence and its limits.

## Scope

Implement native Android adapters in C#; keep music decisions in shared coordinators.
The shell currently targets .NET 10 Android with API 26 minimum. Revisit minimum support only
against physical device results; target SDK and store rules must be checked at release time.

## MIDI and USB

Prefer Android MidiManager discovery and MIDI ports for the class-compliant device.
The adapter forwards raw MIDI bytes; receive timestamps are not yet exposed through IMidiTransport
and sends currently dispatch immediately. Timestamped scheduling is future work. USB event-packet encoding
belongs below this boundary and is not added again to bytes sent through Android MIDI APIs.
Confirm SLOOP via handshake, and show MIDI and audio connection independently.

UsbManager is for host capability/status or a proven fallback, not for claiming the entire
composite USB device by default. Avoid taking audio interfaces away from the system driver.
A raw-USB MIDI fallback requires its own packet codec, interface ownership, permission,
disconnect handling, and simultaneous-audio test. It is not part of the first implementation.

## Audio routes

Enumerate actual input/output routes and subscribe to changes. Start with AudioRecord for
capture and a replaceable preview backend. Choose an input explicitly; a preferred-device
request is not proof of routing. Once recording begins, inspect the routed device and actual
format. If FM1 was requested but a microphone was selected, stop and explain rather than
silently recording another source. Do not automatically switch source mid-take.

Playback separately enumerates and verifies output devices. Prefer the user-selected FM1
USB output when present; a MIDI port or capture input is not evidence of playback support.
The basic MediaPlayer adapter exposes selection on Android 9+, confirms the route while
muted, and stops on route loss/change. See [USB-PLAYBACK.md](USB-PLAYBACK.md).

References: [MIDI APIs](https://developer.android.com/reference/android/media/midi/package-summary)
and [AudioRecord](https://developer.android.com/reference/android/media/AudioRecord).

## Lifecycle and permissions

An application/session service owns live resources; Activity recreation owns only views.
Rotation preserves selection and documents, releases touch-owned notes, and does not restart
capture. A service exposes immutable status to whichever Activity is active.
Request microphone/capture permission only when needed; denial leaves import/editing usable.
Raw USB permission is requested only when a raw-USB operation requires it.

Current capture is foreground-only, limited to two minutes, and finalizes on OnStop.
Performance and MIDI preview release notes on background; performance also releases on
focus loss, track/workspace changes and rotation. No background capture service exists.
The following foreground-service behavior is a future design target.

Background capture will use an appropriately declared foreground service with a visible stop
notification. Start capture from a visible user action; honor while-in-use permission and
foreground-service restrictions for the target Android version. Playback and connected-device
service types are selected only for the services actually running. Notification denial is
handled according to Android behavior rather than assumed to grant or deny capture itself.
Reference: [foreground service types](https://developer.android.com/develop/background-work/services/fgs/service-types).

Performance window-focus loss releases notes. Android audio-focus request/duck/pause handling
for MediaPlayer remains separate work; do not equate window focus with an AudioManager callback.
Keep audio take recovery independent of whether the UI returns. Unplug ends an FM1 take with
an interrupted marker. Process death restores documents/recoverable takes, never reconnects
and starts transport automatically. Dispose ports, unregister callbacks, and stop workers in order.

## Files and diagnostics

Use app-private storage for sessions/assets and the Storage Access Framework for import/export.
Copy chosen source content into the asset store if durable access cannot be guaranteed.
Never treat content URIs as filesystem paths. Report unavailable/revoked providers clearly.
Reference: [document access](https://developer.android.com/training/data-storage/shared/documents-files).

Current status exposes firmware/link, operation failures, routes, capture format/processing
and conversion reports. A consolidated diagnostics screen/export is planned to show OS/device,
app/firmware build, port/route identity, actual audio format,
buffer/dropout counts, protocol latency, and connection failures. Export diagnostics on request;
exclude original recordings and raw sample payloads by default.

## Acceptance

Verify permission denial, rotation, screen lock, audio-focus changes, cable removal, reconnect,
process restart, revoked file access, and low storage on physical devices. Emulator testing is
limited to UI and simulated adapters. USB power/charging and hub compatibility are measured,
not inferred from successful enumeration.

