# Transport handoff — 2026-10-07

Independent transport/capture implementation is complete in the shared local checkout. No
commits, resets, reserved-file edits, firmware work or phone installation were performed.

## Files and APIs

- Sloop.Workstation/PatternLoop.cs: MusicalClock, PatternLoop, TransportTick.
- Sloop.Workstation/PerformanceCapture.cs: PerformanceCapture and CaptureMode.
- Sloop.Android/Services/PerformanceTransport.cs: capture sender adapter, loop task and scene hooks.
- Sloop.Android/Services/DeviceEditing.cs: playback section only; synchronized owned-note cleanup,
  compatibility one-shot PlayNotesAsync delegates to loop and cancels at the next phrase start.
- PerformEditor.cs: record choices/save, capture-aware performer failure subscription, lifecycle
  completion via existing StopPerformance; current-Activity save retry buffer.
- SequenceEditor.cs: continuous Loop control, phase-preserving tempo setter and channel inspector.
- Sloop.Transport.Tests/: new standalone net10.0 console project; no shared Program.cs changes.
- SEQUENCING.md, PERFORMANCE.md and this handoff.

Public connection entry points: LoopPatternAsync(AppPattern, tempo), StopPlaying(),
StartCapture(pattern, mode, tempo, countInBars), FinishCapture(), IsRecording, IsCountingIn,
EnablePerformanceCapture(), SetTransportTempo(tempo), TransportTicked,
QueueTransportBoundary(absoluteTick), CancelTransportBoundary(), TransportEpoch.
EnablePerformanceCapture replaces the inactive existing player once with the same player type
and a wrapped sender; AddPerformEditor migrates its Failed handler. Root code caching the old
PerformancePlayer or attaching other subscribers should use the current getter/rebind them.

## Exact shared integration hooks

1. Add Sloop.Transport.Tests to Sloop.slnx. Production sources compile through the existing
   Workstation and Android default includes; no new production project reference is needed.
2. Existing MainActivity StopPerformance on focus/navigation/background/destruction finalizes
   capture; existing Disconnect releases performer then StopPlaying before closing ports.
   Keep those calls. StopPlaying does not itself finalize capture; hosts without MainActivity
   must FinishCapture and persist its proposal on interruption. Capture and loop remain mutually
   exclusive with existing Perform/preview/device edits. Recording state is not published through
   connection.Changed because shared OnConnectionChanged stops performance on every notification.
3. Existing shared OnConnectionChanged is invoked at loop start/end on the calling UI context.
   Invoke LoopPatternAsync from UI, or marshal connection Changed listeners yourself. TransportTicked
   is explicitly a worker callback. UI playhead rendering must marshal without driving scheduling.
4. Scene bridge: construct Sloop.Scenes.MusicalPosition(tick.TransportEpoch,
   new Tick(tick.AbsoluteTick), tick.TicksPerQuarter, 4, new Tick(tick.PatternLength)).
   Call QueueTransportBoundary(queue.Due.Value) after SceneScheduler.Queue; call Commit at the
   matching TransportTicked callback before new note dispatch, then Reconcile. Stop must cancel
   the scene token and boundary. Restart gets a fresh epoch. Next beat/bar/phrase due arithmetic
   belongs to SceneScheduler; transport inserts exact ticks including non-grid due positions.
   Callbacks must be fast and prevalidated. Root must implement outgoing-note/unsent-event
   cleanup, document restoration, running pattern replacement and tempo application together;
   current loop captures an immutable AppPattern and does not replace it on scene commit.
   Simply invoking StopPlaying inside a callback cancels future notes; it does not switch scenes.
5. Session/preset host may persist existing PerformOptions and generic channel choices through
   its assigned models; no competing shared state model was introduced. Recording currently
   uses PerformOptions.Tempo; loop uses editing.Tempo. A unified transport tempo requires shared
   state wiring and should call SetTransportTempo when changed. Perform settings changes currently
   end the take safely. Session load must stop performance/capture/loop before replacing documents.
6. Generic MIDI mapping still remaps canonical pattern channels 0,1,2,9 through the existing
   output choices; arbitrary captured override channels are retained. Sequence's new edit-channel
   inspector makes all 16 channels editable independently of output mapping.

## Completed behavior and limits

Continuous app phrase looping, timed Perform capture, silent count-in, overdub/channel replacement,
wrap splitting, undoable save, owned MIDI cleanup and monotonic tempo re-anchor are implemented.
No backing loop during recording, audible click, external sync, timestamped lookahead, measured
jitter, swing, scene restore or hardware scene atomicity is claimed. Recording captures app MIDI
before hardware transforms. Crossing-loop notes split/retrigger because the existing AppNote
model prohibits durations past phrase end. Holds beyond a phrase are bounded to one phrase.
Save failures retain proposals only within the current Activity; process recovery is future work.

## Validation

Serial domain build: dotnet build android/src/Sloop.Transport.Tests/Sloop.Transport.Tests.csproj
--no-restore -m:1 --disable-build-servers -nr:false. Run its built net10.0 test DLL directly.
18 checks cover wrap, count-in, replacement/preservation, minimum durations, undo/redo,
actual arp emissions/shared performance ownership, tempo continuity, looping/shared MIDI notes,
stop cleanup, fractional PPQ beats, and exact queued scene due with stable epoch.
Android -t:Compile succeeds with existing explicit AndroidSdkDirectory and JavaSdkDirectory
under C:/Users/shaun/AppData/Local/Android, serial build/server reuse disabled. Existing warnings
in unrelated Fm6Visuals.cs remain. Full APK build reached compiled Android DLL but failed first
at AAPT pipe closure, then LLVM typemap input-file discovery during concurrent root packaging.
Per root coordination, further Android builds are left to the originating chat. No APK installation was attempted.
Physical FM1, simultaneous touch and latency acceptance remain for root deployment.


## Follow-up: discoverable sequence recipe entry

SequenceEditor.cs now places Describe a sequence · offline recipes near the top. It opens a
bounded supported-phrase entry/choice dialog, then reviews existing AppEditor.Propose diffs
before Apply locally or Discard. Scope is the selected edit channel; exact snapshot validation,
local persistence and undo remain unchanged. Four fixed recipes are supported; unsupported
phrases report an error. No model packages or network services were introduced. SEQUENCING.md
reflects the UI. Per root build coordination, this follow-up leaves Android verification to the
combined root build; existing transport domain checks remain unaffected.
