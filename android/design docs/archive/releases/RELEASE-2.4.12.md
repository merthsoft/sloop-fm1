# Latest firmware — 2.4.12 Merthsoft

October 8, 2026. Adds leased phone fill/punch controls, the seven hardware drum groove
starters and shared phone groove-bank access, plus USB return gain/mute/diagnostics.
Existing chord/arp latch and modifier behavior remains supported. INFO reports protocol 12;
extensions negotiate capabilities individually. Atomic hardware scenes remain disabled.

- Package: `build/update-2.4.12/firmware/sloop-2.4.12-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `9d8bf9b6dd498a287c1e1d954b3010b2beaf286b72344f863979c7e06831143b`.
- [Local installer](http://127.0.0.1:8782/webapp/installer/).
- Target image 577,608 bytes; static RAM 93,332/98,304; pool 334,560/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Existing audio ISR budgets fail.
- Android also adds persistent FM6 bank saving; updated APK installed on Pixel 7a.

No firmware flash was performed by this chat. See [verification](../../VERIFICATION.md)
for scope and [previous release](../../archive/releases/RELEASE-2.4.11.md) for history.
