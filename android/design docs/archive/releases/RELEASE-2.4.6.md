# FM1 firmware 2.4.6 Merthsoft

October 8, 2026. In chord mode with LATCH enabled, non-CHROM black-key modifiers now
reshape the retained chord without another root press. Modifier release restores the chord.
The retained root is tracked separately from released keyboard keys; changed tones receive
note-offs/ons internally and over MIDI while common tones continue ringing. CHROM retains
literal black-key roots. No parameter IDs or project layout changed.

- Package: `build/sloop-2.4.6-Merthsoft.fwsc`, 610,066 bytes, identity `FM-1_900`.
- SHA-256: `3ec4f56683329f5bfc294019afe32a25de6d11d4f267b8efa41068dcd377318e`.
- Installer: `http://127.0.0.1:8776/webapp/installer/`; site `build/update-2.4.6`.
- Target checks pass: image 571,340 bytes, static RAM 92,948/98,304,
  pool 334,560/344,064, RAM-text 925 instructions/no calls, register access clean.
- Keyboard harness passes live minor and combined seventh/sus4 modifiers, release
  restoration, latch OFF cleanup, existing latch tests, chromatic roots and scale regressions.
  UI harness and 20,000-frame fuzz pass. Served version and package hash verified.

The revised Android APK is installed on Pixel 7a. App and followed hardware octave changes
leave latched chords unchanged until the next chord press. Offline Pixel checks verified
Oct+ retaining 48/52/55 until I is pressed (60/64/67), and Oct− retaining 60/64/67 until
I is pressed (48/52/55), with latch enabled throughout. Android build: zero warnings/errors.
Firmware has not been flashed; audible hardware acceptance remains pending.
