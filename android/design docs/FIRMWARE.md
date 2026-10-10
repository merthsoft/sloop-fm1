# Firmware companion extensions

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

## Current firmware

Merthsoft.14 adds selected-synth vibrato/tremolo HOME locks: hold LFO/ENV, tap
HOME, then release. Keyboard and knobs remain usable; HOME or a function button
unlocks. STOP/panic, menus and track changes clear the lock. Runtime only, with no
new static RAM or wire/project format. See the [illustrated firmware guide](../../docs/firmware/MERTHSOFT-FEATURES.md).

Production firmware is **2.5 Merthsoft.14**, with INFO protocol **14**, three synth tracks and
one drum track. Protocol 9 FM6/native-pattern exchange, sample-slot backup/upload, USB audio
playback and MIDI note performance are integrated. Protocol 10 command 43 reports the
physical octave offset; Android follows it without firmware transposing ordinary external MIDI.
Hold an already sounding physical chord, then hold ARP or SEL (SLOOP: SCL) for 700 ms to toggle chord latch once without releasing or retriggering notes. This works with ARP either off or on. Releasing keys after enabling latch sustains the chord or keeps its arpeggio running. Disabling latch keeps physically held notes sounding until key release. New key presses, layer knobs, HOME, another layer, track changes or releasing the chord cancel the shortcut; without a held chord, layer behavior is unchanged.

SCL 2 knob 4 controls chord latch; CHROM uses literal keyboard roots, while other modes
allow modifiers to reshape a latched chord immediately. With chord latch enabled, modifier
presses toggle each quality until pressed again, including across new root chords. Releasing
a modifier preserves it. Qualities are owned per synth and clear on latch disable, CHROM,
STOP or panic. With ARP enabled, toggled qualities also revoice the retained arp chord after physical root release; modifiers update the arp note pool without restarting the arp clock. With latch disabled modifiers remain momentary.

Latest target sizes and memory headroom are recorded in [release notes](RELEASE-NOTES.md). RAM text is 925 instructions with no calls.
The existing CPU budget failures were not relaxed; they remain an engineering follow-up.
User hardware acceptance is recorded in STATUS, independently of measured deadline evidence.

Firmware accepts its upstream standard MIDI CC map for track parameters. Android XY macros emit standard MIDI CC; unsupported controllers and pitch bend remain outside this slice. Protocol 14 retains remote performance (73), drum grooves (74), USB return controls (75) and relocated SYN kits (80–84), and adds leased musical starter discovery/audition/application (76). Arpeggios now preserve source velocity across audio, generated MIDI and recording.

Merthsoft.8 adds a persistent scale-note LED guide: physical SEL + OCT+ toggles it
without changing octave or latch. Merthsoft.9 aligns it and the held SEL display with
the selected synth's SEL-page ROOT/SCALE, suppressing generic keyboard backlight
while active. It dims scale keys
behind bright pressed notes and excludes drum/grid and control layers. Settings bit 22
defaults off for older settings and uses the existing deferred flash-save path.

Merthsoft.11 replaces the physical wheel gesture with temporary selected-synth
LFO/vibrato and ENV/tremolo panels. Rate, depth, waveform and beat sync are runtime-only;
release restores the prior page. MIDI/Android CC1 remains available. No wire or
project format changes. See [performance modulation](../../docs/firmware/MODULATION-WHEEL.md).

Merthsoft.13 adds scale-aware grid labels and optional persisted MIDI SCALE
keyboard pitch following. OFF remains appropriate for Android's already-mapped
note output. No Android/wire changes. See [scale/MIDI grids](../../docs/firmware/SCALE-MIDI-GRIDS.md).

## Atomic hardware scenes: remaining requirements


The command-72 schema/client and bounded RAM staging implementation are tested scaffolding.
Production defaults to `FELUCCA_SCENE_TX=0`: the handler/buffer are compiled out, INFO reports 14 for other extensions,
and live atomic application is unavailable. Experimental builds report zero capabilities and
Unsupported Begin until all engine hooks exist. Existing stopped-only sound transfer remains.
Wire contract: [SCENE-TRANSACTION.md](../src/Sloop.Protocol/SCENE-TRANSACTION.md).

Enabling this feature requires all of the following:

1. Install every `sc_hooks` callback (`revision`, `validate`, `apply`, `enter`, `leave`,
   `schedule`) before USB servicing. Do not advertise capability until the complete path works.
2. Track a nonwrapping u28 revision across panel/editor writes, bank/preset/sample changes,
   recording, project adoption and engine switches. Exhaustion disables capability until a
   fresh baseline; stale host snapshots cannot manufacture a new revision.
3. Validate FM6 patch fields/macros, native steps/micro/fill/lock limits, duplicate locks,
   divisions and actual sample dependencies. Resolve native references to immutable payloads.
   Flash replacement and drum-track replacement are outside the current scene transaction.
4. Provide IRQ-safe descriptor exclusion and ordering. CRC/schema work stays in the main loop;
   queued payload stays immutable until apply/cancel. Hooks must not yield, send USB or block.
5. Establish an absolute musical u28 tick/epoch domain; `song.tick` is an audio-block counter.
   Schedule a strictly future beat/bar/phrase callback before sequencer events. Detect crossed
   or late ticks and fail closed rather than leaving a transaction queued indefinitely.
6. Implement bounded, infallible apply after validation: release outgoing sequencer notes and
   locks, swap patches/macros/native synth patterns/tempo, reset scheduling origins and publish
   revision together. Audit patch setters and worst-case deadlines; no flash, allocation or USB.
7. Reset staging on stop/seek/restart/project adoption/host loss and bounded inactivity expiry.
   Publish Applied only after completion; duplicate Commit must not repeat application.
8. Re-measure enabled static RAM, stack, image and callback timing. The earlier enabled build
   used 96,052/98,304 RAM, versus 92,948 production; the staging struct alone is 3,104 bytes.
9. After those checks, negotiate scene capabilities independently of INFO and wire Android native serialization, boundary
   UI and observed-status reconciliation. Queued is not Applied; epoch changes cannot prove
   an old outcome. Retain stopped-only fallback without silently using it during playback.

No atomic sample flash upload, transparent retry after an ambiguous timeout, persistent
transaction status across reboot, multiple queued scenes or whole-device scene swap is claimed.

## Product decision (existing contract)

Firmware changes are authorized. Build app and device as one instrument while preserving
existing editor and project compatibility. New features must fit measured RAM, flash, USB,
CPU, and audio deadlines. The remaining families propose semantics; the implemented scene
staging extension reserves command 72 in Sloop.Protocol/SCENE-TRANSACTION.md.

Implementation checkpoint: firmware USB playback is implemented with UAC1 adaptive
EP4 OUT, clock matching, bounded buffering, ramps and instrument-only capture. Focused
host tests and the target build pass. Hardware testing is accepted by the user for now. Negotiated return gain/mute/diagnostics are integrated; physical audio validation for these new controls remains to be measured. The shared host runner
includes playback tests; static diagnostics now include uac_play_mix. Existing audio ISR
budgets still fail for baseline and playback, and no budget was raised. See
[USB-PLAYBACK.md](USB-PLAYBACK.md) and the
[implementation note](../../docs/firmware/USB-PLAYBACK-IMPLEMENTATION.md).

## Capability and operation contract

Add discoverable feature flags and limits, firmware build identity, protocol revision,
connection epoch, and state revision. Negotiate optional extensions after existing INFO.
Older hosts continue using existing commands. Unsupported requests receive explicit errors
in the new envelope, rather than timeout being the only discovery mechanism.

New operations use request IDs, structured status, and a bounded duplicate-result cache for
safe retransmission within an epoch. Distinguish accepted, queued, applied, busy, invalid,
unsupported, canceled, and failed. Acknowledging a queued operation is not proof it executed.
Applied events include operation ID and actual musical boundary/revision. Reboot invalidates
request caches. Exact widths, ranges, frame sizes, and IDs must be specified in the repository
protocol document before implementing either endpoint.

## Extension families

| Family | Proposed operations |
| --- | --- |
| Transport | Start, continue, stop, query, tempo, record mode and per-track arm |
| Position | Clock authority, running state, bar/beat/tick, phase and discontinuity |
| Sections | Capture section, query availability, quantized launch and cancel queued launch |
| Chains | Read/edit bounded chain, repeats, enable/disable and current/next section |
| Performance | Fill held/next bar, punch FX press/release, explicit all-notes cleanup |
| Transactions | Prepare/validate/commit/cancel bounded sound or scene delta |
| Change stream | All-track revisioned changes, subscription masks and gap detection |
| Transfer | Future transfer identity, offset verification and safely repeatable chunks |

Record audio remains a phone operation; firmware arm refers to its pattern recorder.
Do not emulate panel button sequences for remote commands. Route new commands through the
same validated musical operations used by the panel so behavior remains coherent.

## Real-time application

Parse and validate in the main loop. Stage a bounded delta outside the audio interrupt.
Queue a compact descriptor for the engine to apply at its own audio/musical boundary.
The callback performs bounded work only: no flash, USB waits, or heap allocation.
Choose a safe ownership handoff for staged storage; the producer cannot reuse it until applied
or canceled. Engine changes and voice cleanup need explicit audio-safe transitions.

Scene preparation checks slot dependencies, supported parameters, lock capacity, and current
revision. Commit applies either the complete validated change or none; device-side edits
between preparation and commit cause conflict or explicit rebasing, never partial application.
Initially support one prepared scene and one queued launch; broader queues require budget proof.
High-rate position telemetry is optional, rate-limited, and lower priority than audio and notes.

## Flash and recovery

Never write flash on scene boundaries or performance gestures. Persistent save is a separate
operation allowed only in a safe stopped state. Reuse existing validation and A/B commits.
Current SMP_BEGIN invalidates a slot; atomic sample replacement requires additional storage
and cannot be claimed without a flash-layout budget. A resumable transfer extension needs
durable transfer metadata or an explicitly limited live-session guarantee.

On host loss release host-owned notes, momentary fill/punch state, and incomplete prepared
transactions. Distinguish host-owned and panel-held gestures. Specify latch behavior separately;
do not clear the musician's hardware notes as an incidental disconnect effect.

## Acceptance and rollout

Inventory every visible control against existing remote support first. Add capabilities and
transport/position before scene transactions. Update protocol docs, firmware tests, and app
fixtures together. Measure worst-case callback time, image size, static RAM, queue occupancy,
and USB audio continuity. Validate existing web editor, old projects, backup/restore, panel
operation, and app coexistence. Firmware installation remains a separate explicit user action.



## Integrated performance and groove extensions

Command 73 uses bounded leases for remote fills/punch; physical punch controls take priority.
Stop, panic and project adoption clear remote ownership. See [performance wire](../src/Sloop.Protocol/PERFORMANCE-WIRE.md).
Command 74 discovers the same sixteen ROM groove starters used by the hardware GROOVE page
and applies ordinary editable drum steps while stopped. Existing drum material requires confirmation;
replacement shares hardware undo and does not change the kit or global tempo.
See [drum groove design](../../docs/firmware/DRUM-GROOVES-DESIGN.md).
Sequencer-to-arp routing is active through ARP 2 ORD SNOTE/SPLAY: literal recorded
chords contribute independently of the legacy live held list. Direct sequenced chord playback
is suppressed while ARP is enabled in these modes. NOTE/PLAY retain legacy behavior; ARP OFF
restores direct playback. TIE retains the sequence snapshot, REST/fill rejection clear it,
and STOP/panic/project adoption release generated output. This uses 18 bytes of bounded state.
The general ownership table remains inactive; local/USB/TRS same-pitch ownership and
source/generated MIDI collisions retain their existing limitations. See
[harmony design](../../docs/firmware/CHORD-ARPEGGIO-DESIGN.md).
