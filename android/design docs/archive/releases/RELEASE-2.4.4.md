# FM1 firmware 2.4.4 Merthsoft

October 8, 2026. Adds **QNT / KEYS = CHROM** on SCL: all piano keys trigger literal chord
roots, including black keys. Existing values 0–2 retain scale-degree/modifier behavior.
OCT/TRN apply; ROOT does not transpose literal keys. CHORD controls quality; scale-derived
shapes use tonic scale quality at each root. SCALE CHR retains its legacy minor-chord fallback.
Select CHROM + MAJOR to play C# major from C# directly. Web editor includes the new enum.

- Package: `build/sloop-2.4.4-Merthsoft.fwsc`, 610,066 bytes, device `FM-1_900`.
- SHA-256: `8ab1b29ece9f556d9b51492fb5414718c6485eda0e6ab6e13e56302324c1dd73`.
- Site: `build/update-2.4.4`; local installer `http://127.0.0.1:8774/webapp/installer/`.
- Target build passes RAM/image/pool/register/RAM-text checks: image 570,716 bytes,
  static RAM 92,932/98,304, pool 334,560/344,064, RAM-text 925 instructions/no calls.
- Firmware scale/chord harness passes literal mapping, C# chord recording/release, and
  legacy black-key modifiers. Project capture/apply preserves CHROM without format changes.
  UI harness and 20,000-frame fuzz pass. Download hash and installer identity are verified.

Android companion follow-up pins automatically voiced roots to the selected register:
Later Android follow-up (October 8): octave changes now leave latched notes unchanged until
the next chord press. The newer APK is installed and verified on Pixel 7a; see PERFORMANCE.md.
The original immediate-revoicing verification below records the earlier build.

I→vii→I remains predictable and I ↑ keeps its octave. Octave buttons revoice held/latched
notes without clearing latch. Workstation checks: 65 plus 79 kit mappings. The updated APK
is installed on Pixel 7a. Offline device checks verify I→vii→I returns to 48/52/55,
I ↑ emits 60/64/67, and Oct+/Oct− revoice a latched chord between those registers while
keeping latch enabled and velocity 100. The app was restored to octave 3 with latch off.
Audible FM1 playback acceptance remains pending. Firmware has not been flashed.
