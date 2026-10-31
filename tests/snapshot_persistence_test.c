/* SPDX-License-Identifier: GPL-3.0-only */
#define SNAPSHOT_PERSISTENCE_FIXTURE
#include "headroom_commit_test.c"
#undef main

static void captures(unsigned seed)
{
    for(unsigned k=0;k<NPART;k++) {
        snapshot_p[k][0]=(int16_t)(20+seed+k);snapshot_p[k][3]=3;snapshot_p[k][4]=(int16_t)((seed+k)%6);snapshot_p[k][6]=(int16_t)((seed+2*k)%19);
        for(unsigned side=0;side<2;side++)for(unsigned id=0;id<P_COUNT;id++)
            snapshot_values[k][side][id]=(int16_t)((seed+id+side+k)%100);
    }
}
static void matches(unsigned seed)
{
    for(unsigned k=0;k<NPART;k++) {
        assert(snapshot_p[k][0]==(int)(20+seed+k)&&snapshot_p[k][3]==3);
        assert(snapshot_p[k][4]==(int)((seed+k)%6)&&!snapshot_p[k][5]);
        assert(snapshot_p[k][6]==(int)((seed+2*k)%19));
        for(unsigned side=0;side<2;side++)for(unsigned id=0;id<P_COUNT;id++)if(snapshot_param(id))
            assert(snapshot_values[k][side][id]==(int)((seed+id+side+k)%100));
    }
}
static void cold(void)
{
    memset(snapshot_p,0,sizeof snapshot_p);memset(snapshot_values,0,sizeof snapshot_values);
    memset(snapshot_sections,0,sizeof snapshot_sections);snapshot_saved=0;
    memset(proj_slot,0,sizeof proj_slot);autosave_hash=0;
    song.playing=transport_req=0;persist_boot();autosave_resume();
}
static void load(unsigned slot)
{song.playing=transport_req=0;project_load(slot);transport_req=0;}

int main(void)
{
    setup();snapshot_boot();song.sel=0;ui.menu=0;song.playing=transport_req=0;
    /* The byte representation covers every supported captured parameter range. */
    for(unsigned engine=0;engine<NENGINES;engine++) {
        trk[0].eng_req=(uint8_t)engine;
        for(unsigned id=0;id<P_COUNT;id++)if(snapshot_param(id)) {
            const param_desc_t *d=id>=P_E0?track_desc(&trk[0],id):&TP[id];
            assert(d->min>=-128 && d->max<=127);
        }
    }
    host_tracks_init();captures(1);project_save(0);
    assert(!sec_dirty&&!settings_later);matches(1);
    trk[0].p[P_PAN]=10;captures(2);project_save(1);
    assert(!settings_later);cold();matches(2);
    load(0);matches(1);load(1);matches(2);
    /* Section captures are frozen while playing; later edits belong only to
     * the working project. No flash writes occur until the transport stops. */
    captures(3);song.playing=1;unsigned writes=programs;
    section_store(2);assert(programs==writes && sec_dirty&4u);
    captures(4);song.playing=transport_req=0;sections_write();assert(!sec_dirty);
    cold();matches(4);load(2);matches(3);
    /* The fourth slot also preserves partial captures and signed values. */
    captures(9);snapshot_p[0][3]=1;snapshot_values[0][0][P_PAN]=-37;
    project_save(3);cold();load(3);
    assert(snapshot_p[0][3]==1 && snapshot_values[0][0][P_PAN]==-37);
    assert(snapshot_p[1][3]==3 && snapshot_p[2][3]==3);
    /* A/B-only edits must autosave even when the project checksum is unchanged. */
    captures(5);song.playing=transport_req=rec_wait=ft_on=ui.menu=0;
    ui_input_ms=autosave_ms=autosave_checked=0;fm1_ms=100000;
    autosave_tick();assert(!snapshot_pending());cold();matches(5);
    /* Upstream sees its original settings payload length and contents. */
    st_hdr_t h;int c=st_current(OBJ_SETTINGS,&h);assert(c>=0);
    assert(h.len==sizeof(persist_t)+sizeof(dsu_bank_t) ||
           h.len==sizeof(persist_t)+sizeof(dsu_bank_t)+MIDI_PART);
    persist_t p;assert(st_load(OBJ_SETTINGS,&p,sizeof p)>0 && p.magic==PERSIST_MAGIC);
    /* Uncommitted trailer failures cannot publish a new settings copy or
     * advance the successful snapshot hash. Retrying makes the pair durable. */
    const int failures[]={FAULT_TAIL_PROG,FAULT_TAIL_DROP,FAULT_TAIL_CORRUPT};
    for(unsigned i=0;i<sizeof failures/sizeof failures[0];i++) {
        captures(6+i);uint32_t saved=snapshot_saved;
        st_hdr_t before,after;assert(st_current(OBJ_SETTINGS,&before)>=0);
        fault=failures[i];settings_save();assert(settings_later&&snapshot_saved==saved);
        assert(st_current(OBJ_SETTINGS,&after)>=0 && after.seq==before.seq);
        fault=FAULT_NONE;settings_save();assert(!settings_later&&!snapshot_pending());
        cold();matches(6+i);
    }
    /* Association mismatch discards pairs rather than attaching stale captures
     * to a project replaced by an upstream tool. */
    load(0);project_t replacement=proj_slot[0];replacement.t[0].p[P_LEVEL]--;
    replacement.sum=proj_sum(&replacement);proj_slot[0]=replacement;
    load(0);for(unsigned k=0;k<NPART;k++)assert(!snapshot_p[k][3]);
    /* A legacy settings write drops the optional trailer and remains readable. */
    st_tail=0;st_side=0;
    assert(!st_save3(OBJ_SETTINGS,&p,sizeof p,&dsu,sizeof dsu,0,0));
    cold();for(unsigned k=0;k<NPART;k++)assert(!snapshot_p[k][3]);
 /* All packed scope/time/ready combinations, including legacy zero metadata. */
 captures(0);project_t project;memset(&project,0,sizeof project);snapshot_record_t record;
 for(unsigned scope=0;scope<6;scope++)for(unsigned time=0;time<19;time++)for(unsigned ready=0;ready<4;ready++)for(unsigned edge=0;edge<2;edge++) {
  snapshot_p[0][0]=(int16_t)(edge*127);snapshot_p[0][3]=(int16_t)ready;
  snapshot_p[0][4]=(int16_t)scope;snapshot_p[0][6]=(int16_t)time;
  snapshot_capture(&record,&project);memset(snapshot_p,0,sizeof snapshot_p);snapshot_p[0][5]=1;
  snapshot_restore(&record,&project);
  assert(snapshot_p[0][0]==(int)(edge*127)&&snapshot_p[0][3]==(int)ready);
  assert(snapshot_p[0][4]==(int)scope&&snapshot_p[0][6]==(int)time&&!snapshot_p[0][5]);
 }
 record.t[0].ready=3;record.t[0].position=64;snapshot_restore(&record,&project);
 assert(snapshot_p[0][0]==64&&!snapshot_p[0][4]&&!snapshot_p[0][6]);
    puts("snapshot persistence: cold restart, four section slots, queued capture, A/B-only autosave, unchanged upstream prefix, trailer fault/retry and legacy/mismatch rejection PASS");
    return 0;
}
