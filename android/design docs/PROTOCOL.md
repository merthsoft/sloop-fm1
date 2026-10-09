# MIDI and SLOOP protocol client

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

MIDI/octave checkpoint (2026-10-07): Protocol 10 adds read-only command 43 PERFORM_STATE: empty request, signed v14 hardware octave offset (-3..3) reply. INFO advertises 10; existing command layouts remain unchanged. Android queries only protocol >=10 physical SLOOP links; generic MIDI sends no SLOOP queries.

Status: serialized EditorClient and Workstation device adapters are implemented;
checkpoint 2026-10-07. Handshake currently loads INFO, verified level DESC and three synth
levels. FM6, complete native patterns and sample-slot inventory/backup are loaded explicitly
by their editor actions. WATCH, lease renewal, global transport and revision streams remain
planned; state-machine/synchronization sections below describe those fuller targets.

FM6 GET/PUT uses its native 128-byte packed voice directly, without an extra pack7 layer.
Sample backup/write/header payloads use pack7. Native pattern read/send preserves inactive
data, checks a fresh baseline and verifies full readback. FM6 apply also checks macros,
preset and PTCH. Slot replacement requires a CRC-checked durable backup before BEGIN and
validated byte readback afterward. Failure faults the epoch; reconnect/readback is required
instead of blindly retrying an ambiguous command. Multi-command operations are not atomic.

Owned MIDI performance and one-shot preview are integrated; bulk editing requires them to
be released. Note-offs remain available during busy cleanup. Immediate Android sends and
basic monotonic timing are implemented; timestamped prioritization/coalescing is a target.

## Current contract

Source: [editor protocol](../../web/EDITOR_PROTOCOL.md), [editor.c](../../firmware/src/editor.c),
[usb.c](../../firmware/src/usb.c). Current production protocol is 12. Use INFO and DESC rather than assuming
counts, engine order, or engine parameter IDs. Legacy firmware may omit appended fields.

SysEx framing: F0 7D 46 4C command arguments F7. All payload bytes are 7-bit.
Signed v14 values are value + 8192, low seven bits first. Binary pack7 uses a high-bit mask
followed by up to seven low-bit bytes. Strings are zero-terminated ASCII.
Commands without general transaction IDs require one outstanding editor request.

WATCH 3 enables current notifications including other-track mixer changes. Its lease expires
three seconds after the last request; send PING around once per second during otherwise idle
watching. Other-track full sound/pattern changes are not comprehensively pushed today.

## Connection state machine

Disconnected -> Discovering -> Opening -> Handshaking -> Synchronizing -> Ready.
Permission denial becomes actionable failure; unplug becomes Disconnected. Timeout during
handshake permits bounded retries. Timeout during mutation becomes Uncertain, followed by
resource reconciliation. Reconnect creates a new connection epoch and discards old responses.
Match candidate MIDI ports with discovery metadata, then confirm SLOOP identity via INFO;
the advertised Felucca name alone is insufficient. Do not send update-loader commands.

The target full handshake loads INFO, track/global snapshot, metadata and WATCH.
Lazy-load steps and detailed patches when visible rather than flooding the link at startup.
Show Ready for control separately from audio-route availability.

## Codec, requests and scheduling

The byte-stream parser handles fragments, multiple frames, embedded MIDI realtime bytes,
aborted SysEx, malformed lengths, unknown commands, and bounded maximum frame sizes.
Route channel voice/realtime events separately from editor replies and pushes.
Verify echoed arguments (track, parameter, offset) in addition to command where available.
Never discard notifications while waiting for a reply.

Request classes: read, idempotent setting, destructive/bulk mutation. Timeout policies are
per class and based on measured hardware behavior. Read retries are bounded. Mutations with
ambiguous completion require readback; sample erase/write and project commit are not blindly
retried. Without request IDs, a late same-command reply is ambiguous: quiesce and reconcile
before issuing another indistinguishable request, or reopen the session if necessary.

Prioritize releases and transport, then performance notes, control edits, visible reads,
background reads, and bulk transfers. SysEx frames are serialized intact; channel messages
must not be inserted inside a frame. Realtime MIDI bytes may interleave where supported.
Use small firmware-supported bulk chunks so editor work cannot monopolize the cable.
Coalesce parameter changes by destination; preserve the final gesture value. Queue pressure
must never silently discard note-off. On unrecoverable pressure stop performance and clean up.

## Synchronization

Keep an acknowledged snapshot and pending intent. RELOAD triggers targeted/full refresh as
appropriate. Refresh descriptors on engine change. Use selective refresh for missing current
notifications. Do not poll whole patterns during live play. Reconcile a connected hardware
revision with offline local edits by resource; offer use-device, apply-local, or save-both.
The new firmware revision/change stream described in FIRMWARE.md replaces heuristic refresh
when available, while retaining the v9 fallback.

## Acceptance

Use codec vectors from Python/web tooling; malformed input must never mutate state. Test
fragmented replies, interleaved pushes, unknown versions, late acknowledgments, unplug during
write, and lease renewal. Physical tests verify notes during ordinary editing and explicit
restriction of bulk work during performance. Log bounded metadata, not full private audio.


Protocol 12 adds negotiated commands 73 (remote performance), 74 (shared drum groove bank),
and 75 (USB return controls/diagnostics). INFO version alone does not enable command 72 scenes.
See [firmware extensions](FIRMWARE.md) and [groove design](../../firmware/DRUM-GROOVES-DESIGN.md).
