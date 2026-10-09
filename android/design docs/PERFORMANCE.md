# Performance inputs and chord instrument

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

UI refinement checkpoint (2026-10-07): The shared frame now uses a compact connection menu and selected track/workspace highlights. Playing tips and destination checks are expandable, keeping the chord surface and joystick central.

## Implementation checkpoint — 2026-10-07

Perform now has direct key, scale and Oct−/Oct+ buttons. MIDI channel selection is shown only
in generic MIDI mode; SLOOP mode always routes to the selected synth/drum track, ignoring
saved generic channel overrides. Octave naming uses C3=MIDI 48 and
C4=middle C/MIDI 60. Voice leading stays in the selected register rather than shifting the
whole chord down an octave; explicit inversions retain their bass octave. The register readout
shows app base plus FM1 offset. Follow FM1 octave is enabled by default for protocol 10,
polling command 43 every 100 ms outside device operations. A hardware change updates labels;
latched chords retain their sounding notes until the next chord press. Other held notes revoice.
Older firmware falls back to the app octave, with no unsupported query. Generic MIDI uses
only app octave; hardware follow can also be disabled in settings. The protocol-10 firmware
package is built but has not been flashed or tested against physical OCT buttons.

Perform now offers five MIDI surfaces: eight diatonic chord buttons and a quality joystick,
a 24-note scale grid, two chromatic octaves, sixteen GM drum pads, and a quantized scale
ribbon. All use pointer IDs and glissando between cells. Chord/grid/keyboard/drum pads use
the configured velocity without position-dependent jumps. Ribbon retains vertical expression.
The eighth chord is I ↑, the tonic one octave above the first I; voice leading respects
its upper register and bass/inversion/quality follow that octave. The
surface is visually playable offline; it produces sound through a connected physical FM1.
It does not synthesize sound locally or play phone sample buffers.

Voice-leading follow-up (2026-10-08): automatic voicings keep the literal root at its requested
pitch when no explicit inversion is selected. I→vii→I no longer rotates the tonic root up;
I ↑ remains an octave above the returned I. Explicit inversion settings remain intentional.
App Oct−/Oct+ update the register for the next chord without changing a currently latched
chord or clearing latch. Hardware octave-follow uses the same deferred behavior. Non-latched
held notes revoice immediately without rebuilding the surface; configured velocity is retained.
The Workstation runner now passes 65 integration checks plus 79 kit mapping checks.

Chord joystick mappings are center=diatonic, N=seventh, NE=ninth, E=sus4, SE=sus2,
S=minor, SW=major, W=inversion down, NW=inversion up. Center/sector hysteresis reduces
chatter; inversions change once on entering a sector. Quality returns to diatonic on release.
Ninth chords omit the fifth to stay within four chord voices. An optional different synth
track receives bass root. Voice leading selects a nearby deterministic voicing.

Settings include all twelve keys, major/natural minor/Dorian/Mixolydian/harmonic minor and
major/minor pentatonic melody scales, register, inversion, velocity, tempo, rhythmic rate,
strum spacing, voice leading, bass destination and generic MIDI-channel override. Eight-button
chord buttons use major/minor harmony for the corresponding pentatonic melody scales.

Eight play styles are implemented: block, ascending/descending strum, ascending/descending/
bounce/deterministic-random arp, and gated note repeat. Strum and timed modes run on a
worker using monotonic deadlines, independent of view drawing. Their clock is internal;
external sync, measured touch/MIDI latency and audio-thread scheduling are future work.
Latch holds the last chord after finger release; every new chord-pad press or slide to a degree
re-attacks the chord, including pressing the same degree again. Block attacks release the old
voicing and optional bass before sending the new note-ons. Joystick revoicing retains common
tones. Voice lead is a checkbox beside Latch; changing it affects the next voicing without
stopping the current chord. Other surfaces
use ordinary press/release ownership. Release/stop is always accessible above the surface.

Shared pitches stay on until their final owner releases. Block quality changes retain common
tones. Generations cancel pending strums/repeats. Navigation, track/mode/settings changes,
touch cancel, focus/background/rotation and disconnect clear notes and latch. MIDI note-offs
remain permitted during device operations so cleanup cannot be blocked by the busy state.
Stop suppresses movement from already-held pointers until they lift; a fresh press can play again.
Performance/recording and sequence looping/device edits are mutually exclusive.

Check destination reads current firmware transforms and MIDI routing without changing them:
hardware arp/chord/quantize/transpose, MIDI IN=CLOCK and drum/channel mismatches are reported.
Default synth channels are 1–3 and default drum channel is 10; explicit channel override is
available in generic MIDI mode. The firmware currently ignores CC and pitch-bend, so this build provides chord
expression and quantized note slides rather than ineffective bend/mod-wheel controls.

Performance choices survive Activity recreation within the process; persistent presets belong
to session integration. Perform recordings save as ordinary app patterns with undo/redo. Timed playback pauses on cleanup and never silently resumes after reconnect.
HiChord-inspired mappings describe this app's own instrument, not exact HiChord compatibility.

The shared suite now passes 65 integration checks plus 79 kit checks, including a 36,288-voicing matrix,
shared-note ownership, common-tone retention, invalid input, generation cancellation and
complete cleanup and explicit inversion preservation during voice leading. Pixel testing
covers the installed layout, chord touch, latch, joystick C-to-Cmaj7 change and release,
drum note 36, keyboard, ribbon, bounce-arp selection, readable settings and cancellation
clearing a latched C chord. The user considers hardware acceptance complete for now;
these checks do not provide measured worst-case latency or simultaneous-finger coverage.

## Responsibility

Translate touch/controller gestures into owned musical events, independent of UI rendering.
Perform surfaces target the FM1 by default; local sample pads are a separate selectable target.
Key, scale, target track, clock source, and latch status stay visible while playing.

## Chord surface

Eight degree buttons display degree and resolved chord name, including I ↑; a touch joystick has center plus
eight directions. Use a dead zone and directional hysteresis so small movements do not chatter.

The chord surface and compact joystick stay ahead of the collapsed recording section.
A visible ↕ rail alongside each playing surface accepts scroll gestures without striking
notes. Dragging inside pads/joystick remains a musical gesture. Same-workspace/track refreshes
retain scroll position instead of jumping to the top; changing workspace/track resets it.

Pixel follow-up verification: installed and cold-launched the updated APK; visually verified
all eight pads and direct key/scale controls. Upper I reads C 60/64/67 at octave 3, and
scrubbing from the first row to the upper tonic retains velocity 100. The scroll rail
reveals recording/tips without striking notes. Changing C→D preserves scroll position;
C major and latch-off were restored after testing. No physical MIDI output was measured.
The initial assignment proposal is center=diatonic triad; N=seventh; NE=ninth; E=sus4;
SE=sus2; S=minor; SW=major; W=inversion down; NW=inversion up. Treat mappings as presets,
not compatibility promises with HiChord. Repeated entry into an inversion sector changes the
inversion once; staying there does not cycle it. Users can remap the directions.

Resolve degree root from key/scale, choose quality, build intervals, apply inversion/register,
then voice-lead to the previous voicing. Extended chords prioritize root, third/suspension,
seventh, then extension when limited to four notes; disclose omitted notes. A bass destination
can carry root separately while the chord destination retains harmonic tones. Voicing is
deterministic and constrained to MIDI range and destination voice budget.

Modes: block, strum, arp, repeat, latch, bass-plus-chord. Start with block and controlled
quality changes, then timed modes. Explicitly choose app or FM1 arp; prevent both from running
unintentionally. With app-expanded chords, negotiate a plain-note destination setting or show
the FM1 chord/scale transform conflict before performance. Restore temporary settings only
when they still match the app's change; never overwrite a later panel edit.

## Event ownership

Each touch/controller source gets an owner ID and generation. Track owned notes by destination,
channel, pitch, and owner. Aggregate owners prevent one finger releasing a shared note held
by another. Quality changes diff old/new voicings: release removed notes, retain common tones,
start added notes. Since MIDI 1 has no note-instance IDs, same-pitch retrigger behavior must be
explicit; default sustain while any owner holds, with deliberate retrigger as a separate action.

Pending strum/arp events carry an owner generation. Release or mode change cancels future
events before emitting cleanup. Handle ACTION_CANCEL, focus loss, navigation, rotation,
disconnect, and explicit panic. Latch transfers notes to a latch owner and shows a release
control; disconnect clears latch and never silently replays it after reconnect.
Use firmware host-owned cleanup when available; legacy global panic may affect hardware-held
notes and is an explicit emergency action. Sustain pedal ownership needs the same cleanup.

## Other controls

Drum pads support configurable position-to-velocity, gate/one-shot modes, and repeat.
Keyboard and scale grid share the same ownership engine. XY controls and macros emit bounded,
coalesced parameter updates; a macro declares destinations, ranges, curves, and default value.
Punch effects and held fill are press/release actions with disconnect cleanup. External MIDI
routing prevents echo loops and has explicit source, destination, channel and transform rules.

## Acceptance

Test overlapping same-pitch chords, held quality changes, latch transitions, slide across
buttons, canceled touches, mode changes during strum, and disconnect with pending events.
No note remains owned after cleanup. Voicings stay in range and show actual emitted notes.
Measure touch-to-MIDI timing on device; UI animations must not drive musical scheduling.



## Pixel verification — 2026-10-08 (updated octave behavior)

The revised APK passes offline Pixel 7a checks: latched C remains 48/52/55 when Oct+ changes
the display to octave 4, then the next I press produces 60/64/67. Oct− leaves that chord
at 60/64/67 until the next I press returns to 48/52/55. Latch stays enabled throughout.
The app was restored to octave 3 with latch off. This supersedes the immediate revoicing
behavior verified below; audible FM1 playback still requires connected hardware.

### Earlier verification

The updated APK was installed and exercised on Pixel 7a in offline mode. With latch enabled,
I→vii→I produced 48/52/55 → 50/53/59 → 48/52/55; I ↑ produced 60/64/67. Oct+ revoiced
the latched lower tonic to 60/64/67 at octave 4, and Oct− returned it to 48/52/55 at octave 3.
Latch remained enabled and the readout remained LATCHED throughout, with velocity 100.
The app was restored to octave 3 with latch disabled. These checks verify UI and calculated
notes; audible FM1 playback and touch-to-MIDI latency still require connected hardware.

## Gesture recording implementation — 2026-10-07

Perform now includes Record gestures and Stop / save beside the existing Release / stop.
Recording options are overdub (default), replace captured channels, and an optional silent
one-bar count-in. PerformanceCapture observes outgoing MIDI after the existing PerformancePlayer
sender succeeds. Thus strum spacing, arp/repeat gates, latched chords, shared pitches, bass,
channel override and protocol-10 revoicing are recorded as played by the app. Hardware's own
additional MIDI transforms are not observable here. Offline gestures may also be captured
as a silent local pattern; no local audio is claimed.

The capture-enabled performer remains connection.Performer, so existing panic, disconnect,
background and focus cleanup still cancel timed jobs and release owned notes. StopPerformance
finalizes a take through EditingWorkspace.EditPattern; navigation/settings/track changes,
touch cancellation and lifecycle interruptions also save the completed take. Finger-up alone
keeps recording running and latch remains active until explicit release. A hardware octave
change leaves latched chords sounding until the next chord press; other held notes revoice
within the current take. Existing MIDI routing is preserved.

Captured pitch, channel, velocity and unquantized duration become ordinary editable AppNotes;
select Edit notes channel in Sequence to edit override channels. Stop creates one atomic
undoable edit. Replacement affects only channels with captured material and an empty take
does not clear anything. Record and pattern playback remain mutually exclusive. Capture
has no backing-loop monitoring or punch-in; overdub adds to the stored pattern after stopping.
The count-in is timing-only with no audible metronome. Holds spanning a loop split at wrap;
subsequent playback retriggers those segments rather than preserving a tie across the boundary.

A failed local save retains the EditProposal in the current Activity for retry through Stop /
save; new takes are blocked until it succeeds. Activity destruction or process death does not
persist this recovery buffer. Session presets and transport state are integrated, and the
combined APK is installed. Hardware acceptance is complete for now; touch latency has no
new quantified measurement in this checkpoint.



## Configurable controls checkpoint — 2026-10-08

PerformOptions now carries optional Workstation PerformanceMapping and PerformanceMacro values.
Seven lower chord pads can select degrees I–vii; pad eight remains the separate upper tonic I ↑.
Eight joystick directions select any ChordShape; center/release remains diatonic. Settings edit
copies and validate on Apply. Existing settings and old presets default to the original mappings.
Pad velocity and SLOOP selected-synth routing remain unchanged. A latched chord also retains its
original octave during subsequent joystick quality edits until a fresh chord press.
This also applies while the chord finger remains down: joystick revoicing is not treated
as a fresh pad gesture. A new pad press or slide to another pad adopts the displayed octave.
The joystick's visible direction labels and accessibility description reflect its mappings.

An optional XY surface declares selected-track or synth 1–3 destination, each CC, minimum,
maximum, curve and release default. X increases rightward; Y increases upward. Values clamp
to 7-bit ranges; identical quantized values coalesce. One pointer owns both axes; additional
fingers cannot take ownership. Stop suppresses the held pointer until lift, cancellation,
detachment and normal release restore declared defaults. Connection invalidation clears the
owner and attempts cleanup while transport is available; reconnect never replays gestures.
Cleanup attempts both axes even if the first send fails. These are declared defaults rather
than queried hardware baselines. Sustain, channel-mode and RPN/NRPN/data-entry CCs are excluded.
FM1 currently ignores CC, explicitly disclosed beside the macro. Compatible generic MIDI
receivers can use it. No local audio or macro recording is claimed.

Dedicated Sloop.PerformanceControls.Tests passes 3,024 checks, including range/curve sweeps,
invalid mappings/CCs, I ↑ separation, ownership, coalescing, defaults, disconnect forgetting,
and send-failure cleanup. Combined Android build has zero warnings/errors and the APK is installed. Versioned mapping
and macro settings are persisted and validated through SessionPresetCodec for presets and sessions.

Remaining work: CC updates currently coalesce identical
quantized values, with no timer-based rate limit. One two-axis macro is supported;
external controller input remains future work. These implementation results
do not add a new hardware acceptance gate.

## FM1 fills and punch controls — 2026-10-08

Isolated command73/schema1 firmware and typed client modules now implement held fill,
next-bar fill and all sixteen existing punch DSP effects. Discovery requires INFO protocol12;
older FM1, simulator and generic MIDI connections expose no hardware controls. Remote owners
have 750 ms leases renewed every 200 ms, with at most eight simultaneous gestures. Host fill
aggregates with physical fill; physical punch takes priority over the latest host punch.
Host release cannot clear a panel owner. STOP, USB loss/reset, expiry and lifecycle cleanup
release host gestures. Renew never resurrects an expired or stopped gesture.

Perform provides compact held buttons with pointer-up/cancel/detach cleanup and suppresses
held touches after explicit stop until a fresh press. Next-bar fill must stay held through
the boundary and ends on the following bar or earlier cleanup. Press cancellation reconciles
the accepted press before releasing; ambiguous transport failure relies on bounded expiry
and never retries/replays a gesture after reconnect. No local audio is produced.

The dedicated .NET runner passes 46 checks; the C runner passes discovery, over 16,400 malformed
input cases, overlapping owners, panel priority, lease wrap/expiry, USB loss/reset and STOP.
Shared engine/editor and Activity/connection lifecycle hooks remain parent integration work;
their exact instructions and wire contract are in
[PERFORMANCE-WIRE.md](../src/Sloop.Protocol/PERFORMANCE-WIRE.md). No phone/FM1 acceptance
or new measured audio deadline claim is made by these host-only results.

## External controller input — 2026-10-09

Perform's External controller button enables a separate receive-only Android MIDI 1
source with explicit device and output-port selection. Default is off. Literal mode
preserves controller pitches and note-on velocity, routing all incoming source
channels to Perform's selected track (or generic output-channel override). It uses
block notes regardless of touch surface/style; no app key/octave transposition occurs.

Chord-root mode accepts exact pitch classes in the selected seven-note harmony scale,
uses the controller root's register, current key/scale, chord joystick quality,
inversion, play style, tempo/division/strum timing, and optional bass track. Major/minor
pentatonic use major/natural-minor harmony like touch chords. Chromatic roots and the
drum track produce no chord. This bounded mode does not implement controller latch,
automatic voice leading, octave-follow transposition, MIDI clock, pitch bend,
aftertouch, arbitrary CC mapping, or a general chromatic harmony engine. Held controller
voicings stay at their attack-time settings; joystick changes affect the next attack.

The shared PerformancePlayer owns external notes under external:channel:pitch owners,
so touch owners and overlapping chords retain shared pitches. Incoming velocities are
preserved for fresh audible attacks; a pitch already sounding from another owner does
not retrigger/change velocity. Repeated note-ons are counted with one sounding owner
and need matching releases; sustain is per incoming source channel. Note-on velocity
zero is release. CC64, CC120, CC123 and CC121 support sustain, all sound off, all notes
off, and pedal reset respectively. CC120/123 release that source channel immediately,
including sustained notes. Note-off velocity is not forwarded (existing player emits 0).

A byte-stream decoder handles fragments, running status, interleaved realtime and
ignored SysEx/system messages. Input packets are copied and queued on the main Handler;
no workspace redraw occurs on MIDI arrival and no UI lock surrounds output. At most
64 callback batches can be pending; overflow disables input and releases its owners.
Reset discards queued batches and partial decoder state. Track/workspace/mode/settings
changes and release clear ownership; octave register revoice also clears controller
notes. Output connection session changes and backgrounding close input, pending opens time out
or are invalidated, and device removal/flush release notes. New playing requires a
fresh attack; stale releases cannot release touch owners. Timed strums/arps/repeats
cancel through PerformancePlayer. Capture observes resulting output notes via the
existing performer, with the same overlap/velocity limitations as touch recording.

Integration: MainActivity.MidiInput.cs owns UI and controller routing;
Services/AndroidMidiInput.cs owns discovery/open/removal/receive and exposes the
read-only Fm1Connection.MidiDestinationDeviceId and internal MidiDestinationSession via a partial. PerformEditor calls
AddMidiInputControl, ResetMidiInputNotes in StopPerformance/register revoice.
MainActivity synchronizes session identity in OnConnectionChanged, closes input in OnStop and disposes in OnDestroy. Busy/status transitions reset notes through StopPerformance but retain the input port; incoming input is suppressed while CanPerform is false.
Other audition/route integrations must call StopPerformance before taking the output;
no Sequence or firmware preview code is changed by this component.

Hardware-free checks: Sloop.ExternalMidiInput.Tests covers decoder fragmentation,
running status, realtime/SysEx/system common, velocity-zero release, sustain/reset,
repeated/shared touch notes, per-source channel cancellation, velocity preservation,
root-register/key/chord quality and timed strum cancellation with balanced note-offs.
