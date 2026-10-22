/* SPDX-License-Identifier: GPL-3.0-only */
/* Regression for shared display/entry paths, exercised through both callers. */
#define UI_PAGES_HARNESS_ONLY 1
#include "ui_pages_test.c"

int main(void)
{
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();
    /* A previously visited special page must never relabel HOME's macros. */
    for(uint32_t track=0;track<NTRK;track++) {
        song.sel=track;
        for(uint32_t page=0;page<NPAGES;page++) {
            ui.page=page;ui.home=1;ui.force=1;
            for(uint32_t type=0;type<5;type++) {
                song.g[G_LTYPE]=type;draw_columns();
                for(uint32_t k=0;k<4;k++) {
                    int16_t *v;const param_desc_t *d=home_param(k,&v);
                    assert(!strcmp(ui.big_l[k],d->label));
                    char value[12];const char *unit;param_format(d,*v,value,&unit);
                    assert(!strcmp(ui.big_v[k],value));
                }
            }
        }
    }
    /* MIDI and physical keys replace a cell once, then accumulate a chord. */
    for(uint32_t midi=0;midi<2;midi++) {
        host_tracks_init();song.sel=0;song.seq_mode=1;song.rec=0;rec_wait=ft_on=0;
        ui.home=0;ui.cursor=0;ui.entry_open=0;
        for(uint32_t p=0;p<NPAGES;p++)if(PAGES[p].scope==SC_STEP){ui.page=p;break;}
        TSEL->p[P_VOICE]=V_POLY;TSEL->p[P_CHORD]=0;TSEL->p[P_QUANT]=0;
        TSEL->step[0]=(step_t){.time=ST_NOTE,.n=1,.note={90},.lvl=3,.rat=3,.flags=SF_SLIDE};
        TSEL->step[1]=(step_t){.time=ST_NOTE,.n=2,.note={90,100},.flags=SF_CONT0};
        uint32_t session=undo_sess;
        for(uint32_t k=0;k<2;k++) {
            if(midi) {
                mi_ok=1;step_in_lost=0;step_in_r=step_in_w;
                step_in_q[step_in_w++%STEP_IN_Q]=(uint16_t)((60+4*k)|128u);
                midi_entry();
            } else {
                fm1_in.notes=1u<<k;seq_entry(1u<<k);
            }
            assert(TSEL->step[0].time==ST_NOTE && TSEL->step[0].n==k+1);
            assert(TSEL->step[0].note[0]!=90);
            assert(!(TSEL->step[0].flags&SF_TIES));
            assert(!TSEL->step[0].rat&&!TSEL->step[0].lvl);
            assert(undo_sess==session+4);
            assert(TSEL->step[1].n==1&&TSEL->step[1].note[0]==100);
        }
    }
    puts("shared UI paths: HOME context isolation and MIDI/panel chord-entry transactions PASS");
    return 0;
}
