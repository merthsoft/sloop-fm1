# Sampling handoff — October 8, 2026

Implemented offline pitch/root review and manual overrides, per-chop gain/tuning persistence,
processed audition and matching kit conversion/fit/root mapping. Original WAVs stay immutable.
No MainActivity/shared session/solution/central runner/transfer/firmware files were edited.

## Changed files

- Workstation/SamplePreparation.cs: optional chop edits, tuned fit/conversion and actual roots.
- Workstation/SampleKitPlan.cs: optional document/edit mapping; sorted midpoint splits and duplicate rejection.
- Workstation/SamplePitch.cs (new): bounded pitch analysis, validated chop settings codec/rate helpers.
- Android/Services/SampleKit.cs: chop-aware preparation key and conversion snapshot.
- Android/Services/SampleWorkspace.ChopAudio.cs (new): atomic sidecar storage, detection and processed preview.
- Android/SampleEditor.cs: selected controls, processed chop/pad/assignment audition and actual assignment display.
- Android/SampleChopAudioEditor.cs (new): editor wiring, review/apply estimate and manual numeric fields.
- Android/KitEditor.cs: tuned fit meter.
- Sloop.SamplePitch.Tests project and Program.cs (new): dedicated runner, links existing mapping checks read-only.
- design docs/SAMPLING.md and this handoff.

## Parent integration hooks

SDK default source glob includes new Android partials: no MainActivity initialization needed.
Do not change SLOOP-KIT-1: existing session validation only accepts its six/seven-line format.
In SessionWorkspace export's sample suffix list add `.chopaudio`. In staged session validation
(SampleWorkspace.SessionAdoption PrepareAsync), if the sidecar exists, reject files >16,384 bytes,
call `ChopAudioSettings.Parse(File.ReadAllText(source.Path+".chopaudio"))`, and reject records whose
End exceeds source.Frames. The getter loads by current document source path each time; Adopt
needs no cached-state reset or additional KitSettings fields. Archives must preserve sidecar
next to original WAV. Existing sessions without it use default gain/tuning/mapping roots.
Register dedicated project with the solution only if desired; central tests remain untouched.

## Validation

Run:

```powershell
dotnet build android/src/Sloop.SamplePitch.Tests/Sloop.SamplePitch.Tests.csproj -m:1 -v minimal
dotnet android/src/Sloop.SamplePitch.Tests/bin/Debug/net10.0/Sloop.SamplePitch.Tests.dll
```

Final build: zero warnings/errors. Runner: 27 new checks and 79 existing kit mapping checks.
Checks cover sine roots/confidence, silence/noise/short-range defaults, cancellation, opposite
stereo channels, gain, tuned duration/pitch/fit/full loop, actual Q16/root headers, persistence,
changed ranges, duplicates, sorted instrument ranges and byte-for-byte source preservation.
Early parallel MSBuild attempts occasionally exited with no diagnostics; serial -m:1 builds
completed. Android editor/service changes await parent's serialized app compilation.

## Limitations and remaining work

Autocorrelation confidence is periodic similarity, not calibrated certainty. Short, noisy,
polyphonic or harmonic-dominant chops may return no/wrong pitch; review and manual override
remain required. Analysis uses the beginning of the selected chop, not a whole-chop search.
Tuning is speed change (duration and capacity change), not time stretching; source-rate rounding
introduces small tuning error. Root detection is raw-source pitch; tuning and root are separate.
Root override changes actual trigger key even for drums/chops; blank restores mapping defaults.
Duplicate roots reject conversion; existing base root validation remains conservative even when
all roots are overridden. Preview of an oversized single chop fails fit instead of allocating
unbounded derived output. Whole-source playback and original WAV export stay raw.
Range identity is deliberate: boundary changes reset effective edits, rather than attaching
settings to a different slice. Undo can recover old records until a later settings save prunes
them; processing settings themselves have no undo. Sidecar corruption blocks conversion until
explicit Save repairs it. Existing portable archives omit this sidecar until parent integrates
above hooks. Session-adoption staging, APK compilation/touch checks and registration are parent
work; no extra hardware gate is requested. Other sampling backlog stays planned.

