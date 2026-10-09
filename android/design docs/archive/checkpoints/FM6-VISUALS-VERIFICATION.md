# FM6 visual editor and SAVE > New checkpoint — 2026-10-07

Root-owned independent work completed while the transport, sampling, sessions and scenes
chats worked on their assigned files. Their changes remain subject to separate integration
review; this APK includes shared-checkout work available at build time.

## Android

- Visible Sound navigation: Edit sound / Describe a sound. Prompt entry and proposal actions
  are on a dedicated page; optional locks are collapsed. These are deterministic offline
  recipes, not a conversational model.
- Manual controls precede files/device exchange. All 32 algorithm graphs use firmware bus
  routing; macro algorithm overrides are explicitly reflected in the graph and role labels.
- OP1–OP6 selection, carrier output bus and renderer self-feedback sites are drawn. No
  guessed DX7 multi-operator feedback paths are shown where the current engine lacks them.
- Operator envelope levels drag vertically and commit once on release. Rates and levels
  retain native numeric controls with bounded direct entry; horizontal spacing is schematic.
- Copy/swap entire operators preserves other operators, globals and macro context through
  the existing persisted history and invalidation of stale prompt drafts.
- FM6 editing tests: 2,891 checks. Existing sound-design tests: 1,327 assertions.
- Android build: zero warnings/errors. Pixel 7a cold launch: successful (623 ms).
- Phone checks: prompt navigation, graph/envelope visual review, copy destination picker,
  rejected algorithm 0 with algorithm 5 retained, L1 drag 99 → 53 → 99.

Physical FM1 listening, hardware macro interactions and copy/swap readback remain unverified.
Tests do not provide an offline audible FM6 renderer.

## Firmware

Held SAVE adds New at white key 9. Playback/SONG REC must be stopped. First press arms;
second within three seconds during the same hold uses the existing New Project action.
Releasing SAVE or choosing another tile cancels confirmation. Expiration rearms without
clearing. Current loop/default sounds/default tempo reset; saved slots are not erased.
Queued live-section/quick-chain state is cleared.

UI regression checks cover first press, release cancellation, timeout, playing-state
rejection, confirmed reset and absence of slot writes/loads. Existing 20,000-frame UI fuzz
passes; the armed New tile was visually reviewed at 240×240.

Target build passes MMIO/link/RAM checks: image 569,628 bytes, data+BSS 90,612/98,304,
pool 334,560/344,064. Updated packages are build/sloop-2.4.1-Merthsoft.fwsc and
build/sloop-mobile-protocol10.fwsc. No firmware was flashed.
