/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
#undef NDEBUG
#include <assert.h>

static uint64_t hit_hash(const dsnd_t *d, unsigned note, unsigned vel, int tune16)
{
    dsv_t v;
    int32_t out[CTL];
    uint64_t h = 1469598103934665603ull;
    rng_state = 0x1234567u;
    ds_on_sound(&v, d, 0, note, vel, tune16, 4);
    for (unsigned b = 0; b < 256; ++b) {
        ds_render(&v, out, CTL);
        for (unsigned i = 0; i < CTL; ++i) {
            uint32_t x = (uint32_t)out[i];
            for (unsigned j = 0; j < 4; ++j) { h ^= x & 255; h *= 1099511628211ull; x >>= 8; }
        }
        assert(v.f1 >= -150000 && v.f1 <= 150000);
        assert(v.f2 >= -150000 && v.f2 <= 150000);
    }
    return h;
}

int main(void)
{
    host_tracks_init();
    uint64_t unchanged = 0;
    for (unsigned k = 0; k < DS_NKITS; ++k) {
        for (unsigned l = 0; l < DS_LANES; ++l) {
            dsnd_t d; ds_factory_sound(k, l, &d);
            for (unsigned vi = 0; vi < 3; ++vi) {
                unsigned vel = vi == 0 ? 1 : vi == 1 ? 64 : 127;
                unchanged = unchanged * 1099511628211ull ^ hit_hash(&d, DS_LANE_NOTE[l], vel, 0);
                if (d.wave) {
                    unchanged = unchanged * 1099511628211ull ^ hit_hash(&d, DS_LANE_NOTE[l], vel, -384);
                    unchanged = unchanged * 1099511628211ull ^ hit_hash(&d, DS_LANE_NOTE[l], vel, 384);
                }
            }
        }
    }
    printf("unchanged factory TUNE=0 / tonal tuned fingerprint: %016llx\n", (unsigned long long)unchanged);
    /* Captured before the noise TUNE change. Includes all kits/lanes at three
     * velocities and +/-24-semitone renders of every tonal sound. */
    assert(unchanged == 0x003ad3fd47ca4d70ull);

    dsnd_t d = {0};
    d.src = DN_WHITE; d.nlev = 100; d.ndec = 60; d.level = 128;
    d.flt = DF_LP | (20 << 3); d.fcut = 64; d.hpf = 32;
    dsv_t low, mid, high;
    ds_on_sound(&low, &d, 0, 42, 110, -384, 4);
    ds_on_sound(&mid, &d, 0, 42, 110, 0, 4);
    ds_on_sound(&high, &d, 0, 42, 110, 384, 4);
    assert(low.cut < mid.cut && mid.cut < high.cut);
    assert(low.a3 < mid.a3 && mid.a3 < high.a3);
    assert(low.ahp < mid.ahp && mid.ahp < high.ahp);
    assert(hit_hash(&d, 42, 110, -384) != hit_hash(&d, 42, 110, 0));
    assert(hit_hash(&d, 42, 110, 384) != hit_hash(&d, 42, 110, 0));

    /* Byte-sized per-lane tuning covers a wider range than the UI. Check
     * saturation at both filter edges, HPF=0 stays disabled, all SVF modes,
     * and control-rate envelope updates retain the tuned base cutoff. */
    for (unsigned mode = DF_LP; mode <= DF_HP; ++mode) {
        for (unsigned ci = 0; ci < 2; ++ci) {
            d.fcut = ci ? 127 : 0; d.hpf = ci ? 127 : 0;
            d.flt = mode | (31 << 3); d.fenv = ci ? 127 : -128;
            for (int tune = -2048; tune <= 2032; tune += 16) {
                dsv_t v;
                ds_on_sound(&v, &d, 0, 42, 127, tune, 4);
                int cut = v.cut;
                assert(v.a1 > 0 && v.a2 >= 0 && v.a3 >= 0);
                assert(v.ahp >= 0 && v.ahp < 32768);
                assert(d.hpf || v.ahp == 0);
                ds_control(&v); assert(v.cut == cut);
            }
            hit_hash(&d, 42, 127, -2048);
            hit_hash(&d, 42, 127, 2032);
        }
    }
    d.hpf = 1; ds_on_sound(&low, &d, 0, 42, 110, -2048, 4);
    assert(low.ahp == ds_onepole(1));
    d.hpf = 127; ds_on_sound(&high, &d, 0, 42, 110, 2032, 4);
    assert(high.ahp == ds_onepole(127));

    /* Exercise the real shared hit path: the expanded user sound and its
     * packed factory counterpart receive identical nonzero lane tuning. */
    unsigned lane = lane_of_note(42);
    for (unsigned k = 0; k < DS_NKITS; ++k) {
        dsnd_t user[DS_LANES]; ds_factory_sounds(k, user);
        for (int tune = -24; tune <= 24; tune += 24) {
            memset(&drums, 0, sizeof drums); drum_tune[lane] = tune;
            rng_state = 0x1234567u;
            drum_synth_on(42, 110, k + DRUM_SAMPLED, NULL, DS_KITS[k].crush, 1);
            dsv_t factory = drums.ds[0]; int32_t fout[CTL], uout[CTL];
            /* ds.d points into drums, so render before clearing its sound. */
            ds_render(&factory, fout, CTL);
            memset(&drums, 0, sizeof drums); rng_state = 0x1234567u;
            drum_synth_on(42, 110, DRUM_SYN, user, DS_KITS[k].crush, 1);
            assert(drums.ds[0].cut == factory.cut);
            assert(drums.ds[0].ahp == factory.ahp);
            ds_render(&drums.ds[0], uout, CTL);
            assert(memcmp(fout, uout, sizeof fout) == 0);
        }
    }
    puts("noise TUNE: audible spectrum shift, bounds and factory/user parity passed");
    return 0;
}
