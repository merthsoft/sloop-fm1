# FM1 firmware 2.4.2 Merthsoft

Built October 7, 2026 from the current integrated firmware checkout. The unrelated chord/arpeggio chat was writing a design document only; its proposals are not implemented by this release.

## Release artifacts

- Firmware: `build/sloop-2.4.2-Merthsoft.fwsc` (610,066 bytes).
- Device package identity: `FM-1_900`.
- SHA-256: `858046936ea9099ec5d8117db7977e0470b43a574dd3a63d1f67cea5dcd60561`.
- Generated installer and editor: `build/update-2.4.2/`.
- Local update page: `http://127.0.0.1:8772/webapp/installer/`.

The firmware identifies itself as **2.4.2 Merthsoft**. It includes the previously implemented USB audio playback, protocol 10 hardware octave query, SAVE-held New operation, and visualizers present in this checkout. Android loop transport, sessions, scenes, and sampling improvements are app features and do not imply new firmware scene/session commands.

## Verification

- Fresh WSL AC79 target build passed the checked SDK hash, register access, RAM, image size, and `.ram_text` checks. Image: 569,628 bytes; static RAM: 90,612/98,304 bytes; pool: 334,560/344,064 bytes; `.ram_text`: 925 instructions with no calls.
- Real firmware UI harness passed, including New confirmation/cancellation, rejection while playing, preservation of flash slots, and 20,000-frame randomized UI fuzz. Log: `build/release-2.4.2-tests/ui.log`.
- USB playback regression passed for all three CDC configurations, matching plain/serial-off descriptors, and the unchanged MIDI-only update loader. Logs: `build/release-2.4.2-usb/`.
- Installer version, product, and download SHA-256 match the served firmware package; the built application image contains the new version label.

No device was flashed. Physical USB host compatibility and sustained playback timing still need device testing; this release did not change or relax existing CPU budget baselines.

The local HTTP server binds only to `127.0.0.1` on port 8772; it runs hidden with logs at `build/update-2.4.2-http*.log`. Port 8766 unexpectedly returned a previous installer, so this release uses a separate port. The installer uses the existing package loader and browser USB flow. Open it in a browser that supports WebUSB to perform the update.
