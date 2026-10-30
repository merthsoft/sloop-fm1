# Configurable drum-triggered ducking

GLO > DUCK exposes depth, TRIG, RECOV and SHAPE. Held FX + KNOB 3 continues to set depth. All synth engines share the existing track gain ramp; drums remain unducked, and existing chorus/delay/reverb tails keep sounding. New synth sends are ducked. TEXTURE optionally follows the same envelope through its own DUCK switch.

## Triggers and envelope

TRIG selects KICK (lanes 0/1), SNARE (2/8), K+S (both groups), or ANY (all sixteen lanes). `drum_on` checks the canonical GM-to-lane map before user sample remapping, so kit changes do not change trigger identity. Metronome/count-in woodblocks 76/77 are excluded. Synth notes never trigger ducking. The existing `drums.kick` flag now means selected source hit; multiple hits within a control block coalesce into one envelope restart. Sound audition paths that bypass `drum_on` do not trigger it.

RECOV reuses N_DIV/DIV_DEN: quarter, eighth, sixteenth, thirty-second, eighth triplet and sixteenth triplet. The existing musical advance follows tempo, external synchronization and HALF/DOUBLE. A hit restarts elapsed time; changing the recovery division changes the current envelope's duration. The existing block gain interpolation smooths transitions, including live control edits. There is no per-sample source detector or audio-level compressor.

SNAP squares remaining time, RAMP uses it linearly, and HOLD doubles/clamps it before applying depth. Depth uses the stronger range introduced in .97: approximately half reaches the former maximum, while full depth saturates the dip for longer. Gain always stays between zero and unity; deeper settings cannot invert polarity. Default trigger/recovery/shape remain KICK, eighth and SNAP, preserving .97 output at matching depth.

## Persistence

The project layout and size do not grow. The stored G_DUCK word packs depth in bits 0–6, trigger in 7–8, recovery in 9–11, shape in 12–13, and tag 01 in 14–15. The live G_DUCK value remains 0–127. Validated full project adoption decodes each field; old or invalid tags retain clamped legacy depth and default other settings. Invalid recovery/shape values cannot index lookup arrays. Capture/checksum includes all settings, so project saves and working-state autosave persist them. Section adoption leaves these global duck settings unchanged, as it did for depth.

With default KICK/eighth/SNAP controls, capture leaves the legacy depth word untagged. Only nondefault configuration needs the packed tag. This preserves byte-identical upstream fixture resaves at defaults; .98/.99 had tagged even defaults.

## Space trade

The first complete controls build overflowed by 268 B. Removing only the Android-specific command 87 groove-bank handler recovered 320 B in an intermediate link. Native groove browsing, preview, apply and undo remain intact. Android MIDI/chord playing, sample uploads and sampling, remote performance (86), upstream SYN editing and the web editor remain available. Command 87 joins retired 88/89 with an explicit `[op,2]` unavailable reply. The Android client displays a hardware-GROOVE-page message. Its client decoder still supports older firmware that implements the command.

The final normal **2.8.0.98** image is **584,288 / 584,336 B**, leaving **48 B flash**. Main static RAM is **97,612 / 98,304 B**, leaving **692 B**; low RAM **28,928 / 29,392 B** and pool **335,424 / 344,064 B** are unchanged. Compared with .97 this is +32 B linked flash and +32 B main static RAM, including the new controls, persisted settings, handler removal and version packing. No sample compression, voice-budget change, limiter change or new per-sample DSP is used.

## Verification

- `duck_test.c`: all 128 depths and 18 recovery/shape combinations are bounded and recover monotonically; all GM inputs and source selections match lane policy in sampled/factory/user SYN paths; click exclusion, retriggers and old-maximum scaling pass. UBSan runs with the existing fixed-point shift exception.
- `duck_ui_test.c`: real page/knob bindings on all four track selections, clipping-checked drawing, retained held-FX shortcut and screen capture. RAMP fits the five-character value tile.
- `project_test.c`: every trigger/rate/shape combination survives checked import/full adoption; section behavior, old defaults, invalid tags and negative legacy depth are checked.
- `headroom_commit_test.c`: real NOR save and reload after clearing live state preserves all duck controls; retired commands explicitly reply unavailable without writes. Existing sample upload, SYN editing, settings/fault and editor tests remain.

The full UI/navigation/20,000-frame fuzz run passes with the GLO page-count expectation updated from nine to ten. All 200 audio goldens remain unchanged. Fourteen-scene dual-core/all-mask parity and forced-timeout checks pass. Native/WASM audio parity and browser playback/editing tests, 51 Android groove-protocol checks (including the retired capability), and 16 Pages deployment tests pass. Target placement, unchanged audio-loop budgets/inlining guards and the master LOFI sample-loop guard pass. The local installer is verified serving .98. Listening and knob responsiveness on physical hardware remain to confirm.

## Per-track routing and scene-corner status

Each synth FILTER page exposes DUCK on knob 2. `duck_bypass[3]` uses zero for inclusion, preserving legacy behavior. Its block-level branch selects the existing duck ramp or unity before pan and sends; mute/solo still apply. Drums and TEXTURE retain their existing paths. Sound browsing and morphing do not alter the routing switches.

Project metadata extends the existing `G_ENGSEL` delay tag: `0xED00 | delay_type | bypass_mask << 3`, with low three bits reserved for delay types and bits 3–5 for synth bypass. A zero payload stays an untagged zero for upstream byte compatibility. Older delay tags decode unchanged. Project and section adoption restore the routing; new-project reset clears bypass. The serialized layout does not grow.

The HOME corner HUD uses existing palette colors and small opaque text cells. It reports master LP/HP percentage, LOFI type and enabled duck track numbers. The arrangement badge moves to the lower left scene corner to avoid overlap. HOME dirty tracking includes these values, including changes while stopped.

Build 2.8.0.101: image 583,896 / 584,336 B (440 B free); main static RAM 97,692 / 98,304 B. Low RAM and pool allocation remain unchanged. Tests cover per-track dry/send routing, mute, all routing/type combinations through project and section adoption, actual NOR cold-state reload, UI encoder routing and live HUD refresh.
