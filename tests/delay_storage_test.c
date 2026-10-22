#undef NDEBUG
#define main hostsim_main
#include <stddef.h>
#include "hostsim.c"
#undef main
#include <assert.h>
#define PROJ_HOST 1
static uint32_t trk_def_engine(uint32_t i){return i<NPART?i:0;}
#include "../firmware/src/project.c"
static project_t stored;
int main(void)
{
    host_tracks_init();assert(song.g[G_LTYPE]==0);
    for(int mode=0;mode<3;mode++) {
        song.g[G_DDRIVE]=29+mode;song.g[G_DBITS]=6+mode;song.g[G_DHOLD]=mode;
        song.g[G_DTYPE]=mode;song.g[G_RMODEL]=4;song.g[G_RTIME]=73;
        song.g[G_TXSRC]=6;song.g[G_TXLVL]=22;song.g[G_TXRATE]=2;
        song.g[G_LTYPE]=2;song.g[G_VWOW]=99;song.g[G_VTONE]=40;song.g[G_VCRACKLE]=4;
        drum_tune[2]=-3;drum_dec[2]=2;
        proj_capture(&stored);assert(proj_ok(&stored));
        assert((uint16_t)stored.g[G_SAVE]==(DRUM_TW_MARK|drum_tw_sum(stored.t[TRK_DRUM].e)));
        song.g[G_DTYPE]=2-mode;drum_tune[2]=drum_dec[2]=0;
        proj_apply(&stored,1);assert(song.g[G_DTYPE]==mode&&song.g[G_RMODEL]==4&&song.g[G_RTIME]==73);
        assert(song.g[G_LTYPE]==2&&song.g[G_VWOW]==99&&song.g[G_VTONE]==40&&song.g[G_VCRACKLE]==4);
        assert(drum_tune[2]==-3&&drum_dec[2]==2);
        assert(song.g[G_TXSRC]==6&&song.g[G_TXLVL]==22&&song.g[G_TXRATE]==2);
        assert(song.g[G_DDRIVE]==29+mode&&song.g[G_DBITS]==6+mode&&song.g[G_DHOLD]==mode);
        song.g[G_DTYPE]=2-mode;proj_apply(&stored,0);assert(song.g[G_DTYPE]==mode);
    }
    int tags[]={0,1,12,0xED03,0xED04,0xFFFF};
    for(unsigned i=0;i<NELEM(tags);i++) {stored.g[G_ENGSEL]=(int16_t)tags[i];song.g[G_DTYPE]=2;proj_apply(&stored,1);assert(song.g[G_DTYPE]==0);}
    stored.g[G_CLRSEQ]=0;stored.g[G_INITSND]=0;proj_apply(&stored,1);
    assert(song.g[G_DDRIVE]==127&&song.g[G_DBITS]==8&&song.g[G_DHOLD]==2);
    stored.g[G_INITSND]=(int16_t)0xD87F;proj_apply(&stored,1);
    assert(song.g[G_DBITS]==8&&song.g[G_DHOLD]==2);
    puts("delay persistence: all modes, full/section adoption, legacy/invalid defaults, reverb/master LOFI/drum tune coexistence PASS");
    return 0;
}
