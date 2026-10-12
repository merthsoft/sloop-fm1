# Track auto-wah — proposed next feature

User preference: ENV + FX should offer auto-wah, with both repeating LFO movement
and a sweep driven by the selected track's own signal. This is a proposal, not
implemented in the current installer.

The combined hold should resolve to a single temporary panel. LFO mode offers
free or beat-synced motion around the track's filter setting. Envelope mode follows
the selected track's level, opening on stronger attacks and closing as the signal
fades, with smoothing to avoid zipper noise. Other tracks and delay/reverb tails
should not drive its follower. Do not confuse this with USB microphone processing.

Use the existing track filter/modulation path where possible. Controls need depth,
base cutoff and free/synced rate in LFO mode, or attack/release and sensitivity in
envelope mode. The panel must preserve normal keyboard/scale lighting and restore
the previous screen on release. A future apply gesture should state which regular
LFO/envelope destinations it replaces; the existing shared-LFO limitation remains.

SEL + FX auto-pan is another suggested companion gesture, not yet approved or built.
Implement these separately from the reviewed PR 111 RAM/dual-core integration.
