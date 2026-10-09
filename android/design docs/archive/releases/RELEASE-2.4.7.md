# Integrated release — 2.4.7 Merthsoft


October 8, 2026. Android integrates sampling pitch/gain/tuning, touch piano roll, offline
symbolic loop composition, performance mappings/XY macros and reversible FM6 A/B audition.
Session/preset persistence and recovery/protection hooks are integrated. See [VERIFICATION.md](../checkpoints/INTEGRATION-2.4.7.md).

Firmware retains 2.4.6 chord latch/modifier behavior. Experimental atomic-scene staging is
compile-disabled in production to retain RAM headroom; no live atomic scene capability is
advertised. INFO remains protocol 10 and stopped-only scene sound transfer remains available.

- Package: `build/sloop-2.4.7-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `aeb18c59c59f096738e83cb096b08c932251d615fdcb153bf1e432492e99bdba`.
- Installer: `http://127.0.0.1:8777/webapp/installer/`; site `build/update-2.4.7`.
- Target: image 571,340 bytes; static RAM 92,948/98,304; pool 334,560/344,064;
  RAM-text 925 instructions/no calls; HAL register checks pass.
- Firmware scale/chord/latch, project, sequence, scene scaffold/loss/fuzz, USB playback/loader
  variants and UI 20,000-frame fuzz pass. Served package hash and version verified.
- Android combined solution build has zero warnings/errors; all 20 domain runners pass
  after the malformed chop-sidecar fix. Pixel cold launch and bounded new-screen checks pass.

Hardware testing is accepted by the user for now. No firmware flash was performed by this chat.
