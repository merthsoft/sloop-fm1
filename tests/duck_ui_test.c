/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"
int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";
    panel=PANEL_DEFAULT;layers_init();palette_set(4);host_tracks_init();
    uint32_t page=NPAGES;
    for(uint32_t i=0;i<NPAGES;i++)
        if(PAGES[i].fam==FAM_GLO && PAGES[i].id[1]==G_DTRIG)page=i;
    assert(page<NPAGES);
    for(uint32_t part=0;part<NTRK;part++) {
        song.sel=part;open_family(FAM_GLO);ui.page=page;page_entered();ui.force=1;frame();
        for(uint32_t slot=0;slot<4;slot++) {
            int16_t *v=0;
            const param_desc_t *d=page_desc(&PAGES[page],slot,&v);
            assert(d==&GP[PAGES[page].id[slot]] && v==&song.g[PAGES[page].id[slot]]);
        }
        for(int trigger=0;trigger<4;trigger++) for(int shape=0;shape<3;shape++) {
            song.g[G_DUCK]=127;song.g[G_DTRIG]=trigger;song.g[G_DREC]=5;song.g[G_DSHAPE]=shape;
            ui.force=1;frame(); /* Bounds-checked real drawing, including drum selection. */
        }
    }
    song.sel=0;song.g[G_DTRIG]=0;song.g[G_DREC]=1;song.g[G_DSHAPE]=0;
    encs[panel.enc[EN_K2]]=1;frame();assert(song.g[G_DTRIG]==1);
    encs[panel.enc[EN_K3]]=1;frame();assert(song.g[G_DREC]==2);
    encs[panel.enc[EN_K4]]=1;frame();assert(song.g[G_DSHAPE]==1);
    ui.force=1;frame();ppm("duck-controls");
    press(B_FX);encs[panel.enc[EN_K3]]=-1;frame();release(B_FX);
    assert(song.g[G_DUCK]<127 && song.g[G_DTRIG]==1 && song.g[G_DREC]==2 && song.g[G_DSHAPE]==1);
    for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].graph==GR_FILTER)page=p;
    assert(page<NPAGES);
    for(uint32_t part=0;part<NTRK;part++) {
        song.sel=part;ui.page=page;page_entered();ui.force=1;frame();
        int16_t *v=0;const param_desc_t *d=page_desc(&PAGES[page],1,&v);
        if(part==NPART){assert(!d&&!v);continue;}
        assert(d==&DUCK_TARGET && v==&duck_bypass[part] && !*v);
        edit_param(1,1);assert(*v==1);edit_param(1,-1);assert(!*v);
    }
    song.sel=0;go_home();song.g[G_FILT]=-40;song.g[G_LTYPE]=2;duck_bypass[1]=1;
    ui.force=1;frame();ppm("home-corners");
    uint16_t before[240*240];memcpy(before,screen,sizeof before);
    song.g[G_FILT]=0;song.g[G_LTYPE]=0;song.g[G_DUCK]=0;frame();
    assert(memcmp(before,screen,sizeof before));ppm("home-clear");
    puts("DUCK UI: global controls, independent synth switches, drum exclusion and live HOME corner refresh PASS");
    return 0;
}
