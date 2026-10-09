# Latest firmware — 2.5 Merthsoft.7

October 9, 2026. Three parallel feature chats are integrated with the native editing changes.

- STEP: hold either OCT button with knob 2 for whole-octave shifts, or knob 3 for whole-step event moves. Knob 1 remains tie/rest painting. A forward move into the event's own tie run keeps its original end and clears the vacated prefix; other moves shift the full chain. Undo restores notes and metadata. Unmodified controls and OCT taps retain prior behavior.
- Turning the SELECT knob between the musical browser's main/rhythm/pitch pages keeps audition phase and sounding notes. Musical setting changes stop it; leaving the browser releases it. The physical SEL button still opens chord/scale controls.
- Android Sequence exposes the native musical bank through command 76, protocol 14 and explicit capabilities: names/scales, key, register, mode, VLEAD and rhythm controls. Leased non-destructive audition and confirmed stopped-only apply use the firmware renderer/undo. No duplicated phone bank or generator. Closing/backgrounding cancels renewal without faulting the editor connection; disconnect/missing renewal expires the firmware lease.
- Android Perform accepts an opt-in external device/output port, excluding the current output destination. Literal pitches or bounded in-scale chord roots retain attack velocity; sustain, running status, fragmented messages, note ownership and lifecycle cleanup are covered. This initial input mode does not implement controller CC forwarding, chord latch or automatic voice leading.
- Firmware arp output retains live and sequence expression through all modes, octaves, ties, latch and source/route changes. Shared live/sequence pitches use the strongest current velocity; PULSE deduplicates expanded collisions at their strongest velocity. Audio, generated MIDI and recording receive the same resolved attack velocity.
- Live synth/drum MIDI takes ownership after releasing any starter preview. Existing MIDI routing storage also guards audition/apply while external notes are held; it is scanned on requests, not every audio block.

Target image **574,480 bytes**, static RAM **97,412/98,304** (892 bytes headroom), pool **333,948/344,064** (10,116 bytes available), RAM text **925 instructions/no calls**, HAL access check clean. Package **610,066 bytes**, identity **FM-1_900**.
SHA-256: `5383f87299acb314bda595a634879ae75aa1de2fd21ad70d5829db5bdd9c0c30`.

Android solution/APK: zero warnings/errors; all **27** domain runners pass. Native sequence navigation, expression, sequencer, browser and command-76 integration checks pass, as does broad UI/audio regression with **20,000-frame fuzz**. Web protocol/updater tests pass, including SYN relocation for both protocols 13 and 14. Existing target ISR cost-budget failures remain documented and were not relaxed. The app is installed on the Pixel and its cold-start process stays alive without a logged crash; the phone is locked, so new UI/controller/FM1 round-trip hardware checks are not claimed.

Earlier releases: [Merthsoft.6](archive/releases/RELEASE-2.5-Merthsoft.6.md),
[Merthsoft.3](archive/releases/RELEASE-2.5-Merthsoft.3.md), [full archive](archive/README.md).
