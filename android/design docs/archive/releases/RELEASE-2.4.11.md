# Latest firmware — 2.4.11 Merthsoft

October 8, 2026. Modifier keys now toggle chord qualities with ARP and chord latch
both enabled. The retained root can be revoiced after you release its physical key.
Minor, seventh, sus and inversion modifiers update the arp note pool; modifier release
preserves the quality and another press toggles it off. New roots inherit toggled qualities.
Physical note counts remain balanced when chord size changes. CHROM keeps literal roots;
latch disable, STOP and panic clear toggles and retained roots.

Hold an existing physical chord, then hold ARP or SEL (SLOOP: SCL) for 700 ms to toggle
latch without restarting the chord. This remains supported with ARP either on or off.

- Package: `build/update-2.4.11/firmware/sloop-2.4.11-Merthsoft.fwsc`, 610,066 bytes, `FM-1_900`.
- SHA-256: `9c9c4058ad585401f7b94dd78c0b0ae68f938fdaa5ec5500bbe73eb2d3623392`.
- [Local installer](http://127.0.0.1:8781/webapp/installer/).
- Target image 572,364 bytes; static RAM 92,980/98,304; pool 334,560/344,064;
  RAM-text 925 instructions/no calls; HAL register checks pass.
- Real UI/audio harness passes, including modifier toggles after root release, toggles
  while holding a root, chord-size changes, physical counts and 20,000 random frames.
- Scale/keyboard and sequencer suites pass. No hardware flash performed by this chat.

Android integration evidence is in [VERIFICATION.md](../../VERIFICATION.md).
Earlier releases are retained in the [archive](../../archive/README.md).