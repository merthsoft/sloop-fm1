# FM1 firmware 2.4.5 Merthsoft

October 8, 2026. SCL page 2 knob 4 is now **LATCH**. With CHORD enabled and ARP off,
keyboard chords sustain after release and the next root replaces the retained chord.
OFF, CHORD off, enabling ARP, STOP, or panic releases the retained notes and sends MIDI
note-offs. This shares ARP HOLD, retains its parameter ID and project format, and survives
sound preset changes. The web editor exposes the same setting on SCL 2 (named HOLD there).

- Package: `build/sloop-2.4.5-Merthsoft.fwsc`, 610,066 bytes, identity `FM-1_900`.
- SHA-256: `587bb59564d6fa3a7ecdeeec80695684ea324f2d266b8aa185ea8122260d7ae8`.
- Installer: `http://127.0.0.1:8775/webapp/installer/`; generated site `build/update-2.4.5`.
- Target checks pass: image 570,988 bytes, static RAM 92,948/98,304,
  pool 334,560/344,064, RAM-text 925 instructions/no calls, register access clean.
- Scale/keyboard harness passes chord sustain, replacement, OFF and STOP cleanup,
  alongside chromatic roots and existing scale/chord tests. Real UI harness passes knob 4
  ON/OFF mapping and 20,000-frame fuzz. Served version and download hash verified.

Android APK is unchanged from the verified 2.4.4 companion build. Firmware has not been
flashed; audible hardware latch acceptance remains pending.
