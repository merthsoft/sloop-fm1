# SLOOP Mobile workstation design

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

MIDI/octave checkpoint (2026-10-07): Generic MIDI output mode is implemented for Android-enumerated destinations, alongside the SLOOP-specific connection. Performance and app-pattern playback use standard MIDI; patch/native-pattern/sample exchange remains SLOOP-only. Direct octave/channel controls and protocol-10 FM1 octave following are implemented.

Status: accepted architecture with implemented Android workspaces; reconciled October 9, 2026
against 2.5 Merthsoft.7. External MIDI input and native musical starter browsing,
leased audition and stopped-only application are integrated; see the owning
[Performance](PERFORMANCE.md) and [Sequencing](SEQUENCING.md) designs for limits.
Android is the primary platform. The complete product scope below includes future work.

Implemented: manual FM6 editing and bounded offline sound prompts; app/native pattern
editing, continuous MIDI looping and Perform recording; recording, chopping, converted sample preview/export,
backed-up slot transfers/restoration; and the HiChord-inspired performance instrument.
Versioned local documents and accepted sound provenance persist on the phone. See
[VERIFICATION.md](VERIFICATION.md) for validation and [STATUS.md](STATUS.md)
for delivery status. The [scene and arrangement foundations](SCENES.md) implement
typed snapshots, chains/repeats, automation evaluation, persistence and boundary scheduling
contracts. Shared scene/transport integration, arrangement management, automation dispatch
and staged session adoption are integrated. Local synthesis and model inference remain future
work. The user accepts hardware testing for now; see STATUS.md.

See the [component design index](README.md) for subsystem contracts and implementation gates.

Firmware changes are explicitly authorized as part of this product. Design the app and firmware
together for a coherent workstation, rather than treating the existing protocol as a permanent
limit. Preserve compatibility and user data; measure flash, RAM, CPU, and timing budgets before
adding device-side features. USB playback firmware is implemented in this checkout;
other companion protocol extensions remain proposed.

## Purpose

Carry an FM1 and a phone as a complete mobile workstation: play, edit, sequence, record,
chop samples, arrange scenes, and manage the instrument over one USB data connection.
An optional Pocket Operator or HiChord should complement the setup, not be required.
The app must remain useful offline for preparing samples and arrangements.

## Stack and platform strategy

Use C# with .NET for Android and native Android views for the initial application.
Keep music models, editing operations, protocol codecs, and session logic in platform-neutral
C# libraries. Android services implement MIDI, audio routing, storage, permissions, and lifecycle.
Use a small C++/Oboe audio component only when low-latency sample audition requires it.
No managed allocations, disk access, or UI calls in real-time callbacks.

Windows is a future adapter and UI project; iPhone is a possible later adapter and UI project.
Do not promise shared native UI. Reuse the core and protocol; evaluate MAUI for those future
frontends if it reduces work. iOS audio/MIDI routing, USB accessories, background behavior,
and signing require independent validation and Apple build tooling. Oboe is Android-specific;
an iOS audio implementation would use Apple audio APIs behind the same service boundary.

C# is appropriate because native Android APIs are accessible directly. Kotlin offers easier
access to Android examples but does not justify replacing the user's preferred language.
References: https://dotnet.microsoft.com/en-us/apps/mobile and
https://developer.android.com/ndk/guides/audio.

## Hardware and connection boundaries

The phone is USB host; FM1 is a USB peripheral. Validate the actual cable, power behavior,
simultaneous MIDI and audio capture, and disconnect recovery on representative phones.
Do not assume every USB-C cable or phone supports the required host configuration.

USB MIDI plays notes and carries the existing SLOOP SysEx protocol. USB audio captures the
FM1 stereo master at 44.1 kHz. The intended playback destination for app previews and
app-rendered instruments is the FM1 speaker/headphone output over the same USB cable.
The checked-out SLOOP firmware implements UAC1 capture and adaptive playback with
clock matching and mixer integration. Host tests and the firmware build pass; physical
FM1/Android/Windows validation remains pending. Capture excludes phone return.
The app discovers and verifies a playback route independently of
MIDI. System output remains an explicit alternative. See [USB-PLAYBACK.md](USB-PLAYBACK.md).
Direct FM1 monitoring avoids the latency of phone monitoring. Stems require isolated passes.

Pocket Operator audio capture needs a compatible external input/interface; the FM1 USB
capture path is not an external audio input. Analog sync would require suitable hardware and
an explicit routing design. Do not promise MIDI or audio integration for unspecified models.
A physical HiChord may be routed as a controller only after its model and interfaces are
verified. Multiple USB peripherals require a tested hub. The built-in chord surface works
with just the phone and FM1.

USB audio reference: https://source.android.com/docs/core/audio/usb.

## Navigation and visual design

Five workspaces: Perform, Sequence, Sound, Sample, Library. A persistent header shows
connection, clock source, and selected track. Transport provides play, stop, record mode,
tempo, and meters. Four track selectors remain reachable across workspaces.
Use a dark SLOOP-inspired palette, track colors, readable text, and generous touch targets.
Portrait focuses on one editor; landscape/tablets add a mixer or detail panel beside it.
Selection and activity must also use labels and shapes, not color alone.

All five workspaces now have functional controls and a compact shared connection menu,
selected track/tab highlighting and expandable editor sections. Disabled global transport
placeholders have been removed; recording and MIDI playback have workspace-specific actions.
Sampling separates Chop from Send, with waveform cursor dragging, paired editing/audition
actions, capacity display and backup/upload/readback progress. Perform keeps visible
latch/release controls and simultaneous chord/joystick surfaces. Responsive tablet
layouts, a global clock display and complete transport remain design targets.

## Perform and chord instrument

Provide drum pads, keyboard, scale grid, chord surface, XY controls, macros, mixer, fills,
and punch effects. Velocity comes from configurable touch position; pressure is optional
only where the device reports it reliably. Do not assume MPE support.

The HiChord-inspired surface uses seven scale-degree buttons and a central touch joystick
with a neutral position plus eight directions. It is an original SLOOP input layout,
inspired by one-button chords and directional chord changes rather than a compatibility clone.
Reference: https://hichord.shop/ and https://manual.hichord.shop/.

Select key, scale, octave, inversion, and target synth track. Buttons display degree and actual
chord name. Direction assignments offer major/minor overrides, seventh, suspended chords,
extensions, and inversions; current mappings are fixed, with remapping planned. Modes: block chord, strum, arpeggio,
repeat, latch, and bass plus chord routed to separate FM1 tracks. Voice leading minimizes
movement between chords. Changing quality while held sends the note difference, preserving
shared notes. Releasing touch, changing mode, losing focus, or disconnecting releases all
owned notes. Track note ownership/reference counts across overlapping touches.

Keep chord voicings within the device's polyphony and the four-note hardware step format when
recording hardware patterns. Extended voicings require an explicit reduction or app sequence.
Avoid double chord expansion: app-generated chords should use a plain-note destination mode
or warn if the FM1's own chord processing remains enabled. The current performance engine
implements these chord modes plus scale grid, chromatic keys, drum pads and scale ribbon.
Its destination check reports transforms and channel conflicts without changing hardware.
XY parameter macros, fills, punch effects and performance recording remain planned.

## Sequence, scenes, and timing

Hardware pattern mode edits four real FM1 tracks, 64 steps per track, 16 drum lanes, up to
four synth notes per step, ties, slides, hit levels, ratchets, micro timing, fill conditions,
and parameter locks. Show finite lock budgets (currently 24 per track) and valid ranges.
Provide piano roll, drum grid, step inspector, copy/paste, transpose, duplicate, and undo.

App sequence mode supports longer MIDI arrangements, extra lanes, and richer automation.
Extra lanes do not add FM1 engines. Each session chooses one clock authority; prevent
unintentional double playback. Hardware patterns use the FM1 timing engine; app arrangements
use a dedicated scheduler rather than UI timers. Measure timing and latency before promises.

FM1 scenes reference hardware sections A-D. App scenes capture named track/macro settings
and app patterns, optionally referencing a hardware section. Show active, queued, and edited
states. Arrange repeat counts and chains. Sample slots are dependencies, not instantaneous
scene-swappable resources. Quantized atomic scene application requires firmware support.

## Sampling and audio recording

Primary flow: record/import -> trim -> chop -> audition -> map -> fit -> upload -> play.
Capture explicitly selected FM1 USB audio or microphone; show the actual route and levels.
Import via the system file picker. Begin with WAV; add formats through verified decoders.
Other-app capture is later scope subject to Android and source-app permissions.
Retain original recordings and non-destructive edits. Support gain, fades, reverse,
silence trimming, normalization, zoom, and undo.

Slice manually, tap during playback, detect transients, divide equally, or use a tempo grid.
Move, merge, split, discard, and retain slices. Audition on 16 pads. Map a drum kit,
chromatic instrument, or key-per-slice set. Compare original and converted FM1 preview.
Show encoded size, zone count, and slot capacity before upload. Fit operations must disclose
whether they trim, omit slices, or compress time; never silently change the original audio.

Current format: mono 22,050 Hz IMA ADPCM; four 80 KiB slots including 512-byte data offset;
up to 16 zones per slot, roughly 7.4 seconds total per slot. USR3+4 supports a combined drum
kit in current firmware. Compute limits from actual encoded bytes, not duration estimates.

Upload uses SMP_BEGIN, ordered acknowledged chunks, and SMP_END with device CRC validation.
BEGIN invalidates the destination. Back up prior contents before replacement. If interrupted,
retain source and transfer details and offer restart; existing protocol does not guarantee
resume. Do not blindly retry writes at erase boundaries after an ambiguous acknowledgment.
Serialize writes and respect firmware stop/busy requirements; restore only with a deliberate
user action. Display success only after validated completion and refreshed slot information.

Audio take recording, MIDI performance recording, and FM1 sequencer recording are distinct
record modes. Provide takes, count-in, loop playback, trimming, WAV export, and Chop this take.
Use recoverable recording files, explicit audio-focus handling, and a foreground service for
background capture with the Android permissions required by the eventual target SDK.

## Sound and library

Edit every supported engine using device descriptions for labels, ranges, and enums.
Provide envelopes, LFOs, arp, effects, mixer, presets, six-operator FM editing, and DX7 import.
Re-read engine-dependent descriptions after engine changes.
Library holds app sessions, original audio, slice edits, kits, presets, recordings, and complete
device backups. An app session is not the same object as an FM1 project or firmware backup.
Version app documents and preserve source/device format metadata. No account or cloud required.

## Protocol and firmware roadmap

Source of truth: ../../web/EDITOR_PROTOCOL.md, ../../firmware/src/editor.c,
../../firmware/src/usb.c, and ../../tools/sampleio.py. Current editor protocol is v9.
Framing is F0 7D 46 4C cmd args F7; values use signed offset 14-bit encoding and binary data
uses pack7. Existing commands cover parameters, steps, presets, samples, backups, and FM6.

There are no general request transaction IDs. Initially allow one outstanding SysEx request,
parse fragmented messages and interleaved notifications, bound buffers, and distinguish
timeouts from rejection. Coalesce continuous controls. Keep real-time note-offs and transport
responsive while throttling editor traffic; suspend bulk operations during performance.
Use WATCH plus selective refresh because notifications do not cover all track changes.
Acknowledged hardware values are authoritative. On reconnect compare local and device state
and present a reconciliation choice before replaying offline edits.

Inventory all controls before promising full remote coverage. Firmware extensions are needed
for missing transport/arm actions, section capture and launch, quick-chain editing, reliable
position/state reporting, missing fill/punch actions, capability discovery, and prepared atomic
scene changes. Maintain backward compatibility and disable unsupported UI features explicitly.
Do not invent command numbers in the app shell. Flash operations respect playback restrictions.

## Architecture and milestones

Sloop.Core: navigation, file-backed PCM/sample edits and capture journals.
Sloop.Protocol: byte codecs, parsing and the serialized protocol client.
Sloop.SoundDesign / Sloop.Sequencing / Sloop.SampleEncoding: validated domain libraries.
Sloop.Workstation: persistence, device adapters, conversion coordination and performance engine.
Sloop.Android: native editors, MIDI/audio/file services and lifecycle integration.
Future platform projects depend on these libraries; libraries never depend on Android.
Audio uses a narrow service boundary with a possible native implementation per platform.

1. Implemented: navigation, MIDI link/simulator, FM6 and pattern editors, sampling and performance.
2. Hardware: user acceptance is complete for now; revisit new failures or material feature changes.
3. Remaining editors: broader engine controls, richer native inspection and preset banks.
4. Remaining studio: timing telemetry, external clock, fills/punch FX and atomic hardware scenes.
5. Later platforms: Windows/iOS adapters and optional local inference/audio backends.

Validation gates: representative physical phones; timing measurements; long recording with
screen off; sustained notes and overlap cleanup; fragmented SysEx; ambiguous transfer failures;
hardware/app change reconciliation; accurate conversion/CRC vectors against existing tooling.
No USB/audio or Android runtime verification is implied by a successful shared-library build.

