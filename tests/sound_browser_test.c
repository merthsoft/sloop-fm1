/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_PAGES_HARNESS_ONLY 1
#define UI_REAL_UPRESET 1
#include "ui_pages_test.c"
#include "../firmware/src/upreset.c"
static void *up_stage(void) { static up_bank_t staged;return &staged; }
static up_bank_t banks[2];

static void browser(void)
{
    ui.home=0;ui.page=(uint8_t)page_first(FAM_SAVE);
    assert(cur_page()->graph==GR_BROWSE);page_entered();
}
static void turn(uint32_t knob,int32_t steps)
{ encs[panel.enc[knob]]=steps*panel.dir[knob];frame(); }

int main(int argc,char **argv)
{
    outdir=argc>1?argv[1]:"build/host";
    panel=PANEL_DEFAULT;layers_init();host_tracks_init();palette_set(NPALETTES-1u);
    knob_accel_off=1;song.sel=0;
    for(unsigned b=0;b<2;b++)up_bank_xip[b]=&banks[b];
    up_rec_t *rec=&banks[0].r[5];
    rec->used=UP_USED;rec->ver=UP_VER;rec->engine=0;rec->np=P_COUNT;
    memcpy(rec->name,"MY SOFT PAD",12);
    for(unsigned i=0;i<P_COUNT;i++)rec->p[i]=track_desc(&trk[0],i)->def;
    rec->p[P_E0]=2;
    browser();uint32_t rank,count,n,k;
    /* Every category is an exact projection of the stable factory/user list. */
    for(unsigned cat=0;cat<=BROWSE_LAST;cat++) {
        browse_category=(uint8_t)cat;browse_list(0,&rank,&count);
        unsigned expected=cat==0?NBANK+1:cat==1?0:cat==NELEM(BANK_KIND)+2?1:0;
        if(cat>=2&&cat<NELEM(BANK_KIND)+2)
            for(unsigned j=0;j<NBANK;j++)expected+=BANK[j].kind==cat-2;
        if(cat>=BROWSE_ENGINE)for(unsigned j=0;j<NBANK;j++)expected+=BANK[j].e==cat-BROWSE_ENGINE;
        assert(count==expected);
        for(unsigned j=0;j<count;j++) {
            n=browse_list(j,&rank,&count);
            if(cat>=2&&cat<NELEM(BANK_KIND)+2)assert(BANK[n].kind==cat-2);
            if(cat==BROWSE_USER)assert(n==NBANK);
            if(cat>=BROWSE_ENGINE)assert(BANK[n].e==cat-BROWSE_ENGINE);
        }
    }
    /* Empty favourite category never changes the patch or opens a transaction. */
    browse_category=1;int16_t empty[P_COUNT];memcpy(empty,TSEL->p,sizeof empty);
    browse_step(1);assert(!browse_saved.active&&!memcmp(empty,TSEL->p,sizeof empty));
    browse_category=0;fav_toggle(1);browse_category=1;
    browse_list(0,&rank,&count);assert(count==1);browse_step(500);assert(browse_saved.active);
    browse_end(0);fav_toggle(0);

    /* Cancellation returns edited parameters, A/B captures and edited FM6 payloads,
     * without replacing the sequence or another track's sound. */
    for(unsigned engine=0;engine<NENGINES;engine++) {
        host_preset(&trk[0],engine,0);browser();browse_category=0;
        TSEL->p[P_REL]=91;TSEL->p[P_DLY]=33;TSEL->p[P_VOICE]=V_POLY;
        int16_t before[P_COUNT];memcpy(before,TSEL->p,sizeof before);
        for(unsigned a=0;a<2;a++)for(unsigned id=0;id<P_COUNT;id++)snapshot_values[0][a][id]=(int16_t)(id+a);
        snapshot_p[0][0]=64;snapshot_p[0][3]=3;
        int16_t ab[2][P_COUNT],morph[4];memcpy(ab,snapshot_values[0],sizeof ab);memcpy(morph,snapshot_p[0],sizeof morph);
        uint8_t patch[FP_SIZE+1u];memcpy(patch,fm6_patch[0],sizeof patch);
        patch[0]=(uint8_t)(patch[0]==50?51:50);fm6_set_patch(0,patch);
        memcpy(patch,fm6_patch[0],sizeof patch);
        step_t notes[NSTEP];memcpy(notes,TSEL->step,sizeof notes);
        int16_t other[P_COUNT];memcpy(other,trk[1].p,sizeof other);
        browse_step(2);assert(browse_saved.active);
        browse_step(-3);browse_end(0);
        assert(TSEL->eng_req==engine && TSEL->preset==0 && !TSEL->user);
        assert(!memcmp(before,TSEL->p,sizeof before));
        assert(!memcmp(ab,snapshot_values[0],sizeof ab)&&!memcmp(morph,snapshot_p[0],sizeof morph));
        assert(!memcmp(patch,fm6_patch[0],sizeof patch));
        assert(!memcmp(notes,TSEL->step,sizeof notes)&&!memcmp(other,trk[1].p,sizeof other));
    }
    browser();song.playing=1;
    turn(EN_K2,2+BK_PAD);assert(browse_category==2+BK_PAD&&browse_saved.active);
    uint32_t total,cur=preset_pos(&total);n=cur;assert(BANK[n].kind==BK_PAD);
    turn(EN_PRESET,1);assert(browse_saved.active);
    ui.hot_t=0;ui.msg_t=0;ui.force=1;ui_draw();ppm("sound-browser-pads");
    turn(EN_K3,1);assert(!browse_saved.active);
    int16_t kept[P_COUNT];memcpy(kept,TSEL->p,sizeof kept);
    go_home();assert(!memcmp(kept,TSEL->p,sizeof kept));
    browser();turn(EN_K1,1);turn(EN_K3,-1);assert(!memcmp(kept,TSEL->p,sizeof kept));
    assert(!browse_category&&!browse_saved.active);
    browser();turn(EN_K1,1);track_select(1);assert(!memcmp(kept,trk[0].p,sizeof kept));
    track_select(0);browser();turn(EN_K1,1);open_family(FAM_ENV);
    assert(!browse_saved.active&&!memcmp(kept,TSEL->p,sizeof kept));
    /* Actual user records participate, and external PC wins over an audition. */
    browser();edit_param(1,NELEM(BANK_KIND)+2);assert(TSEL->user==6);
    edit_param(2,-1);assert(!TSEL->user&&!memcmp(kept,TSEL->p,sizeof kept));
    browse_step(2);pc_req[0]=1;pc_bank[0]=0;pc_poll();
    assert(!browse_saved.active);unsigned e=base_at(0,&k);
    assert(TSEL->eng_req==e&&TSEL->preset==k);
    /* Real input: opening a held-function sheet cancels before it can save. */
    browser();turn(EN_K1,1);press(B_SAVE);assert(!browse_saved.active);release(B_SAVE);
    song.playing=0;
    puts("sound browser: exact category projection, wrap, favourites/users, live audition/keep/cancel, edited patches/A-B, page/track exits and external PC PASS");
    return 0;
}
