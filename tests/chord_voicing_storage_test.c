/* Track harmony survives section/project adoption without changing FUN6 or P IDs. */
#undef NDEBUG
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i) { return i < NPART ? i : 0; }
#include "../firmware/src/project.c"

static const int16_t INVERSION[] = {0, 1, 2, 3, -1, -2, -3};
static project_t stored, imported, neutral;

static void expect_codes(const unsigned code[3])
{
    for (unsigned i = 0; i < NPART; i++) {
        unsigned c = code[i] <= 20u ? code[i] : 0u;
        assert(trk[i].chord_inv == INVERSION[c % 7u]);
        assert(trk[i].chord_spread == (int16_t)(c / 7u));
    }
    assert(trk[TRK_DRUM].chord_inv == 0 && trk[TRK_DRUM].chord_spread == 0);
    assert(song.g[G_NEWPRJ] == 0);
}

static void stale_runtime(void)
{
    song.g[G_NEWPRJ] = 0;
    for (unsigned i = 0; i < NTRK; i++) {
        trk[i].chord_inv = -3;
        trk[i].chord_spread = 2;
    }
}

static void adopt_both(const project_t *p, const unsigned code[3])
{
    for (int all = 0; all <= 1; all++) {
        stale_runtime();
        proj_apply(p, all);
        expect_codes(code);
    }
}

int main(void)
{
    const unsigned zero[3] = {0, 0, 0};
    _Static_assert(NPART == 3 && G_NEWPRJ == 31 && P_COUNT == 71, "stable harmony IDs");
    _Static_assert(sizeof(project_t) == 3816 && offsetof(project_t, g) + 2u * G_NEWPRJ == 70,
                   "unchanged FUN6 carrier layout");
    host_tracks_init();
    for (unsigned i = 0; i < NTRK; i++)
        trk[i].chord_inv = trk[i].chord_spread = 0;
    proj_capture(&neutral);
    assert(proj_ok(&neutral) && neutral.g[G_NEWPRJ] == 0);

    /* Every setting on each track, with the other tracks neutral. Test actual
     * import validation before adoption, not only direct access to the record. */
    for (unsigned track = 0; track < NPART; track++) {
        for (unsigned code = 0; code <= 20u; code++) {
            unsigned wanted[3] = {0, 0, 0};
            proj_apply(&neutral, 1);
            trk[track].chord_inv = INVERSION[code % 7u];
            trk[track].chord_spread = (int16_t)(code / 7u);
            wanted[track] = code;
            proj_capture(&stored);
            assert(proj_ok(&stored));
            assert((uint16_t)stored.g[G_NEWPRJ] == (code ? 0x8000u | (code << (5u * track)) : 0));
            assert(proj_import(&imported, &stored, sizeof stored));
            assert(!memcmp(&stored, &imported, sizeof stored));
            adopt_both(&imported, wanted);
        }
    }

    /* Keep every extension carrier live simultaneously. Distinct track values
     * make a bad shift or accidental global application visible. */
    const unsigned mixed[3] = {4, 12, 20};
    for (unsigned i = 0; i < NPART; i++) {
        trk[i].chord_inv = INVERSION[mixed[i] % 7u];
        trk[i].chord_spread = (int16_t)(mixed[i] / 7u);
    }
    song.g[G_DTYPE] = 2; song.g[G_DDRIVE] = 35; song.g[G_DBITS] = 5; song.g[G_DHOLD] = 4;
    song.g[G_RMODEL] = 4; song.g[G_RTIME] = 73;
    song.g[G_LTYPE] = 3; song.g[G_VWOW] = 99; song.g[G_VTONE] = 40; song.g[G_VCRACKLE] = 14;
    song.g[G_TXSRC] = 6; song.g[G_TXLVL] = 22; song.g[G_TXTONE] = -13;
    song.g[G_TXRATE] = 2; song.g[G_TXMOVE] = 71; song.g[G_TXWHEN] = 1; song.g[G_TXDUCK] = 1;
    drum_tune[2] = -3; drum_dec[2] = 2; drum_tune[15] = 7; drum_dec[15] = -4;
    song.g[G_NEWPRJ] = 1; /* Live GO must never become the serialized payload. */
    proj_capture(&stored);
    assert((uint16_t)stored.g[G_NEWPRJ] == (0x8000u | 4u | (12u << 5) | (20u << 10)));
    assert(proj_ok(&stored));
    for (int all = 0; all <= 1; all++) {
        proj_apply(&neutral, 1);
        stale_runtime();
        proj_apply(&stored, all);
        expect_codes(mixed);
        assert(song.g[G_DTYPE] == 2 && song.g[G_DDRIVE] == 35 && song.g[G_DBITS] == 5 && song.g[G_DHOLD] == 4);
        assert(song.g[G_RMODEL] == 4 && song.g[G_RTIME] == 73);
        assert(song.g[G_LTYPE] == 3 && song.g[G_VWOW] == 99 && song.g[G_VTONE] == 40 && song.g[G_VCRACKLE] == 14);
        assert(song.g[G_TXSRC] == 6 && song.g[G_TXLVL] == 22 && song.g[G_TXTONE] == -13);
        assert(song.g[G_TXRATE] == 2 && song.g[G_TXMOVE] == 71 && song.g[G_TXWHEN] == 1 && song.g[G_TXDUCK] == 1);
        assert(drum_tune[2] == -3 && drum_dec[2] == 2 && drum_tune[15] == 7 && drum_dec[15] == -4);
        proj_capture(&imported);
        assert(!memcmp(&stored, &imported, sizeof stored));
    }

    /* A tag is protected by the project checksum, including through import. */
    stored.g[G_NEWPRJ] ^= 1;
    assert(!proj_ok(&stored));
    assert(!proj_import(&imported, &stored, sizeof stored));

    /* Unknown individual codes must not wipe or contaminate valid neighbors. */
    for (unsigned track = 0; track < NPART; track++) {
        for (unsigned bad = 21; bad < 32u; bad++) {
            unsigned wanted[3] = {4, 12, 20};
            wanted[track] = bad;
            stored = neutral;
            stored.g[G_NEWPRJ] = (int16_t)(0x8000u | wanted[0] | (wanted[1] << 5) | (wanted[2] << 10));
            stored.sum = proj_sum(&stored);
            assert(proj_import(&imported, &stored, sizeof stored));
            adopt_both(&imported, wanted);
        }
    }
    const unsigned absent[] = {0, 1, 0x7FFF};
    for (unsigned i = 0; i < NELEM(absent); i++) {
        stored = neutral;
        stored.g[G_NEWPRJ] = (int16_t)absent[i];
        stored.sum = proj_sum(&stored);
        /* Old GO=1 may survive as a clamped live action on a full load; it is
         * still not a voicing setting and capture must explicitly write zero. */
        for (int all = 0; all <= 1; all++) {
            stale_runtime();
            proj_apply(&stored, all);
            for (unsigned k = 0; k < NTRK; k++)
                assert(trk[k].chord_inv == 0 && trk[k].chord_spread == 0);
            proj_capture(&imported);
            assert(imported.g[G_NEWPRJ] == 0);
        }
    }
    stored = neutral;
    stored.g[G_NEWPRJ] = (int16_t)0x8000u;
    adopt_both(&stored, zero);
    stored.g[G_NEWPRJ] = (int16_t)0xFFFFu;
    adopt_both(&stored, zero);
    puts("chord voicing storage: 63 settings, full/section isolation, metadata coexistence, invalid/absent tags, checksum and neutral carrier PASS");
    return 0;
}
