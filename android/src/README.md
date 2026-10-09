# SLOOP Mobile source

Original Android work uses the [Unlicense](../UNLICENSE). GPL-derived sample encoding
and factory patches retain their licenses; distributing the combined application must
meet applicable GPL requirements. See [licensing scope](../LICENSING.md).

Android-first C# workstation with native views and shared domain libraries. Start with
[current status](<../design docs/STATUS.md>), [design index](<../design docs/README.md>) and
[verification](<../design docs/VERIFICATION.md>) for feature scope, checks and deployment.

Perform provides chords, keyboard, scale grid, drums, ribbon, arp/strum/repeat, latch,
configurable mappings and an XY MIDI CC macro. SLOOP routes by synth; generic MIDI supports
other receivers. The simulator exercises a limited protocol surface and produces no audio.

Sequence provides looping MIDI playback, performance recording, multi-note piano-roll edits,
native FM1 pattern exchange, scenes/arrangements and stepped CC automation. Offline composition
generates editable drafts with MIDI audition, local saving and tempo/progression/rhythm controls.
It uses procedural rules; no language model is included. Drafts are outside session archives,
with portable draft export/import through a separate reviewed archive flow.

Sample imports/records PCM16 WAV, browses retained assets across saved sessions and supports
trim, manual/equal/transient/tap chopping, undo, pitch/root review, gain/tuning and decoded
ADPCM preview. FM1 uploads use durable backups, journals and readback. Named sessions retain
source assets and validated settings. Combined USR3+4 and time stretching remain future work.

Sound provides manual FM6 editing, bounded sound prompts and reversible hardware RAM A/B
audition and persistent FM6 base-voice bank saving. Broader engines and atomic live hardware
scenes remain future work. USB preview verifies explicit output routes; negotiated FM1 return
gain/mute/diagnostics are available. User hardware acceptance is complete for now. See the
current verification record for the exact APK installation and test checkpoint.

## Build

Requires .NET 10 SDK, the Android workload, and its supported Android SDK/JDK setup.
Install these using Visual Studio's .NET Android tooling or the official .NET Android setup.
From this directory:

```powershell
dotnet workload restore Sloop.slnx
dotnet build Sloop.slnx
dotnet build Sloop.Android/Sloop.Android.csproj -t:Install
```

The install target requires an authorized Android device or emulator. An emulator can inspect
the shell but does not validate FM1 USB/audio compatibility.

For repeatable direct phone installation with the default SDK/JDK paths below, use:

```powershell
.\Install-Phone.ps1
```

Unlock the USB-connected phone and authorize debugging first. For multiple devices, pass
`-DeviceSerial`. Custom SDK/JDK locations use `-AndroidSdkPath` and `-JavaSdkPath`.
The project embeds managed assemblies in Debug APKs, so direct ADB installation does not
depend on IDE fast-deployment files. The helper starts fresh build processes to avoid stale
SDK detection after dependency installation.

If SDK/JDK discovery fails, install project dependencies into explicit paths and keep passing
those paths to build/install (the installation flag accepts Android SDK development licenses):

```powershell
$androidSdkPath = "$env:LOCALAPPDATA\Android\Sdk"
$javaSdkPath = "$env:LOCALAPPDATA\Android\Jdk"
dotnet build Sloop.Android/Sloop.Android.csproj -t:InstallAndroidDependencies "-p:AndroidSdkDirectory=$androidSdkPath" "-p:JavaSdkDirectory=$javaSdkPath" -p:AcceptAndroidSDKLicenses=True
dotnet build Sloop.slnx "-p:AndroidSdkDirectory=$androidSdkPath" "-p:JavaSdkDirectory=$javaSdkPath"
```

Shared libraries can be built without the Android workload:

```powershell
dotnet build Sloop.Core/Sloop.Core.csproj
dotnet build Sloop.Protocol/Sloop.Protocol.csproj
dotnet run --project Sloop.Protocol.Tests
```

Future Windows/iOS frontends should reuse Core and Protocol and supply their own platform services.
See https://learn.microsoft.com/en-us/dotnet/android/ for tooling requirements.

## Validation status

Run all platform-neutral domain runners from the repository root:

```powershell
./android/scripts/Test-Domain.ps1
```

The script discovers each `*.Tests` project, reports failures without skipping later runners,
and writes per-project logs plus `results.json` under `build/android-domain-tests`.
Use `-NoBuild` only after building the same configuration; the default builds each runner.

Protocol tests cover encoding vectors, signed-value bounds, pack7 groups, fragmented/interleaved
SysEx, malformed frames, INFO version compatibility, matching/serialized replies, ambiguous
timeouts, and disconnect. Simulator checks validate every generated descriptor, firmware
clamping, track isolation/selection, delayed/dropped replies and unplug behavior.
APK packaging succeeds with the installed SDK/JDK when fresh build processes are used.
Debug assemblies are embedded to avoid the startup abort caused by direct installation of
fast-deployment APKs. On 2026-10-07, direct installation and startup passed on the connected
phone. The simulated handshake, Sound navigation, and level change from 104 to 58 with
acknowledgment/readback passed on that phone. System-bar insets keep navigation clear of
the gesture area. Physical FM1 USB/audio behavior remains unverified.

Sampling validation on the same phone: system-picker import of a 2-second mono WAV,
waveform positioning, manual split at 1 second, Android media playback at 22,050 Hz,
process-restart recovery of the asset/chops, and system-picker WAV export passed.
Exported format and PCM bytes matched the original exactly. Automated tests include
odd RIFF chunks, stereo extrema, exact frame extraction, truncation/unsupported formats,
slice coverage, marker rejection, trim/undo, edit restoration and capture recovery (400 total checks).

Explicit audio-output chooser and verified playback through Pixel 7a BuiltinSpeaker passed
on the phone. The routing build has zero warnings/errors. USB output unplug/change behavior
and actual FM1 playback remain unverified; the firmware playback task owns that return path.

Microphone recording passed on Pixel 7a at 48 kHz mono: normal Stop retained an 88.19-second
take, backgrounding retained a 2.52-second interrupted take, and deliberate process termination
recovered 2.05 seconds of written PCM on restart. WAV adoption and source/processing metadata
were present; previous originals remained. The phone used the disclosed voice-recognition
fallback. Runtime permission, elapsed/peak display and recording screen-awake behavior passed.
The two-minute cap, all input-route failure cases and sustained FM1 USB capture still need
broader physical validation. Recovery retains complete frames written before termination.

