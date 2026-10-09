# Perform controls handoff — October 8, 2026

> Reconciled October 9, 2026: External controller input and sustain ownership are now integrated in bounded literal/in-scale chord modes. Controller CC forwarding, chord latch and automatic input voice leading remain future work. See [Performance](../../PERFORMANCE.md).

## Implemented files
- android/src/Sloop.Android/PerformEditor.cs: optional Mapping/Macro fields on existing PerformOptions; mapped labels/chords; configurable joystick; stop cleanup; retained latch octave for joystick edits.
- android/src/Sloop.Android/PerformanceControlEditor.cs: settings and single-owner XY surface with scrolling rail.
- android/src/Sloop.Android/Services/PerformanceMacros.cs: dedicated connection partial, CC sender and connection invalidation cleanup.
- android/src/Sloop.Workstation/PerformanceControls.cs: validated persistable mappings/macros and owned macro sender.
- android/src/Sloop.PerformanceControls.Tests/{Program.cs,Sloop.PerformanceControls.Tests.csproj}: dedicated executable runner.
- android/design docs/PERFORMANCE.md and this handoff.

No shared integration/codec/solution/central runner files changed. Existing Performance.cs model
remains authoritative; dedicated controls extend it without introducing a competing performer.

## Exact parent persistence integration
SessionPresetCodec.Encode should add versioned keys perform.mapping.v1 and perform.macro.v1
(or consistent existing codec key names), JSON-serializing o.Mapping ?? new PerformanceMapping()
and o.Macro ?? new PerformanceMacro(). Decode absent keys to null/defaults. Deserialize into
these same Workstation records, call Validate(), reject malformed values through existing staged
session/preset validation, then return the existing PerformOptions with Mapping and Macro set.
Never silently accept wrong array lengths or enum values. Copy arrays when exposing mutable
settings. Session JSON directly serializing PerformOptions will already carry these fields,
but dictionary-based saved presets still require the codec additions above.

Mapping schema: Degrees int[7] each 0..6; Directions ChordShape[8] in N/NE/E/SE/S/SW/W/NW order.
Upper tonic is implicit cell 7 and cannot be remapped. Macro schema: Enabled bool; Track -1
(selected track/generic override) or 0..2 (synth channels); nullable X/Y axes default to CC1/74.
Axis schema: Controller, Minimum, Maximum, Default ints; Curve Linear/Squared/SquareRoot.
Controller 0..119 excluding 6,38,64,96..101; minimum <= default <= maximum <=127.
Axes must use different controllers. Bounds and enums are checked by Validate.

## Hooks / lifecycle
Existing StopPerformance now clears joystick and XY pointers, then connection.Macros.Stop().
MainActivity lifecycle/navigation already calls StopPerformance; no new shared hook required.
Dedicated Fm1Connection partial subscribes Changed once and stops/forgets macros when CanPerform
becomes false. XY Move checks CanPerform; reset sends bypass the busy gate so cleanup is possible.
Physical disconnect cannot guarantee delivery of defaults; local ownership always ends, no replay.
Generic selected destination respects PerformChannel; SLOOP synth destinations remain channels 1–3.
CCs are not captured by the note-only performance recorder or added to scene automation.

PlayPerformance now accepts optional freshChord=true. Internal joystick/register revoicing
passes false; pad gestures keep the default. latchedOctave records the last actual chord
gesture, so quality edits preserve the sounding octave even with a finger still held after
Oct−/Oct+. A new chord press or slide adopts the displayed octave. Stop clears latchedOctave.

## Validation and limitations
Run: dotnet build android/src/Sloop.PerformanceControls.Tests/Sloop.PerformanceControls.Tests.csproj
then dotnet android/src/Sloop.PerformanceControls.Tests/bin/Debug/net10.0/Sloop.PerformanceControls.Tests.dll
Result: 3,024 checks; zero build warnings/errors. Quantized duplicate updates coalesce; there is
no timer-based rate cap or interpolation worker. UI event sampling drives CC updates, not notes.
Only one two-axis macro exists. UI controls were source-reviewed, not Android-compiled/deployed;
parent owns final Android compile/APK integration. Persisting dictionary presets requires parent
codec edits. Hardware acceptance is already recorded; no repeat acceptance request is made.
Fills, punch effects, external controller input and sustained-pedal ownership remain later work.
