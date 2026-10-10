# Latest firmware — 2.5 Merthsoft.12

October 9, 2026. Preserve keyboard lighting during performance modulation.

Holding LFO/vibrato or ENV/tremolo keeps the selected-scale guide, bright played
notes and normal backlight preferences. These playable panels do not add grid
landmark lights. Other control layers retain their existing indicators.

Focused scale-light regression passes across every root/scale and backlight
mode/level, plus actual LFO/ENV holds and releases with the guide on and off.
Target build passes: image **576,008 bytes**, static RAM **97,508/98,304**
(796 bytes remaining), pool **333,948/344,064**, RAM text **925 instructions/no calls**.
HAL access check passes. Package **610,066 bytes**, identity **FM-1_900**.
SHA-256: `45bc0f09c4cbd6fce96e1f32dfebf5e62c6a0c488277f662f17a86a816bb0dc5`.

No physical LED validation or new ISR deadline measurement is claimed. Android and
wire/project formats are unchanged. Existing ISR cost-budget limitations remain.

See [scale lights](../../../../docs/firmware/KEYBOARD-SCALE-LIGHTS.md) and
[previous release](RELEASE-2.5-Merthsoft.11.md).
