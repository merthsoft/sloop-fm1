# FM1 firmware 2.4.3 Merthsoft

Built October 7, 2026 from the latest workspace firmware source. Firmware version is
`2.4.3 Merthsoft`. This release does not imply atomic hardware scene switching.

- Package: `build/sloop-2.4.3-Merthsoft.fwsc`, 610,066 bytes, identity `FM-1_900`.
- SHA-256: `b3e25a5804ae4a04541462231ac549c97e3a89c19c77b85f2151d0f7f12ca88f`.
- Installer/editor site: `build/update-2.4.3`.
- Local installer: `http://127.0.0.1:8773/webapp/installer/`.
- Hidden local HTTP server binds to `127.0.0.1`, with logs in `build/update-2.4.3-http*.log`.

Fresh target build passes SDK hash, register access, image/RAM/pool and RAM-text checks.
Application image: 569,900 bytes. Static RAM: 90,868/98,304 bytes. Pool: 334,560/344,064.
RAM-text: 925 instructions, no calls. UI harness and 20,000-frame fuzz pass; USB playback
passes in all three CDC configurations. Served installer version, device identity and
download SHA-256 match the final package. Firmware was not flashed.
