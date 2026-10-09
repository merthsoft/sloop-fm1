/* Command 75, schema 1. Included after editor reply helpers, with usb.c visible.
 * Replies always begin suboperation,status (0 OK,1 malformed,2 unavailable).
 * Diagnostics are bounded individual 32-bit observations, NOT an atomic multi-counter
 * instant. Reset advances counter baselines; it never changes stream/DSP state.
 * Counters wrap modulo 2^32. Extrema remain lifetime values until device reboot. */
#define ED_USB_PLAYBACK 75u
static void ed_usb_u32(uint32_t v)
{ uint32_t i; for (i=0;i<5;i++) { ed_b(v); v >>= 7; } }
#if FELUCCA_UAC
static uint32_t ed_usb_base[8];
static void ed_usb_counters(uint32_t *v)
{
    v[0]=up.packets; v[1]=up.frames; v[2]=up.malformed; v[3]=up.overruns;
    v[4]=up.underruns; v[5]=up.hw_errors; v[6]=up.starts; v[7]=up_clipped;
}
#endif
static int ed_usb_playback_handle(uint32_t cmd, const uint8_t *a, uint32_t na)
{
    uint32_t sub=na ? a[0] : 127u, i;
    if (cmd != ED_USB_PLAYBACK) return 0;
    ed_b(sub);
    if (!na || sub>3u || (sub!=1u && na!=1u) ||
        (sub==1u && na!=1u && na!=4u) ||
        (sub==1u && na==4u && (a[1]>127u || a[2]>32u ||
          (a[2]==32u && a[1]) || a[3]>1u))) { ed_b(1); return 1; }
    if (!sub) {
        ed_b(0); ed_b(1); /* schema */
#if FELUCCA_UAC
        ed_b(15); /* gain, mute, diagnostics, baseline reset */
#else
        ed_b(0);
#endif
        ed_b(0); ed_b(32); /* maximum Q12 gain: 4096, unsigned14 */
        ed_b(0); ed_b(2); /* ramp frames: 256, unsigned14 */
        return 1;
    }
#if FELUCCA_UAC
    ed_b(0);
    if (sub==1u) {
        if (na==4u) uac_play_control(a[1] | (uint32_t)a[2]<<7, a[3]);
        uint32_t c=up_control; ed_b(c & 127u); ed_b((c>>7)&63u); ed_b((c>>13)&1u);
    } else {
        uint32_t v[8]; ed_usb_counters(v);
        for(i=0;i<8;i++) {
            if(sub==3u) ed_usb_base[i]=v[i];
            ed_usb_u32(v[i]-ed_usb_base[i]);
        }
        ed_usb_u32(up.fill_lo); ed_usb_u32(up.fill_hi);
        uint32_t fill=up_w-up_r;
        ed_usb_u32(fill>UP_N ? UP_N : fill); /* bounded observation across preemption */ ed_usb_u32((uint32_t)up.step);
        ed_b(up.alt); /* host selected alternate; not proof of audible routing */
    }
#else
    (void)i; ed_b(2);
#endif
    return 1;
}
