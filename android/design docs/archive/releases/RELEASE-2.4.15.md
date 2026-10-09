# Latest firmware — 2.4.15 Merthsoft

October 8, 2026. Adds opt-in recorded-chord arpeggiation: enable ARP, then choose
ARP 2 ORD SNOTE/SPLAY. Recorded notes drive the arp without duplicate direct playback.
TIE retains harmony; REST clears the sequence contribution; live input stays separate.
NOTE/PLAY keep legacy behavior. Full source-owner aggregation and followers remain future work.

Android adds cross-session sample library browsing/reuse and portable composition draft
archives with reviewed import. Updated APK installed; all 25 domain runners pass.

- [Local installer](http://127.0.0.1:8785/webapp/installer/).
- Package: `build/update-2.4.15/firmware/sloop-2.4.15-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `0411f7cb1205ecbf1931577656593b522439f466047d67a98e58517cd5b9e2c6`.
- Image 580,296 bytes; RAM 93,444/98,304; pool 334,560/344,064.
- RAM-text 925 instructions/no calls and HAL checks pass. Existing ISR budget failures remain.
- Retains sixteen groove starters and non-destructive looping hardware preview.

No firmware flash was performed by this chat. See [verification](../../VERIFICATION.md)
and [previous release](RELEASE-2.4.14.md).
