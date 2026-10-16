/* SPDX-License-Identifier: GPL-3.0-only */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/core.h"
static int32_t clamp(int32_t x,int32_t lo,int32_t hi) { return x<lo?lo:x>hi?hi:x; }
static const uint16_t SCALE_MASK[]={0xFFF,0xAB5,0x5AD};
#define NSCALES NELEM(SCALE_MASK)
static uint8_t transport_req,rec_wait,ft_on,panic_req,sync_reload;
static struct { uint8_t force; uint32_t step_sess; } ui;
static struct { uint32_t sess; } undo;
static uint32_t undo_sess,marks,captures,note_ons,note_offs;
static track_t *marked;
static void undo_mark(track_t *t,uint32_t s) { marked=t;undo.sess=s;marks++; }
static void starter_undo_capture(track_t *t) { assert(t==marked);captures++; }
static uint32_t trk_len(const track_t *t) { return t->p[P_SLEN]>0 ? (uint32_t)t->p[P_SLEN] : 1u; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static uint32_t step_fill(const track_t *t,uint32_t i) { return(t->fill[i/4]>>((i%4)*2))&3; }
static struct { uint32_t notes; } fm1_in;
static void trk_note_on(track_t *t,uint32_t note,uint32_t vel) { (void)t;assert(note<128&&vel<128);note_ons++; }
static void trk_note_off(track_t *t,uint32_t note) { (void)t;assert(note<128);note_offs++; }
#include "../firmware/src/rhythm_shapes.c"
#include "../firmware/src/sequence_starters.c"

int main(void)
{
    uint32_t g,i,j,mode,scale; rhythm_shape_t shape={0,0,0,255,0};
    track_t untouched;
    assert(NSEQUENCE_STARTERS==24);
    /* Frozen before the custom renderer refactor: 100k deterministic ROM events.
     * Covers shaped rhythms, all modes, voicing and saturated register bounds. */
    uint32_t seed=1234567,hash=2166136261u;
    for(uint32_t q=0;q<100000;q++) {
        seed=seed*1664525u+1013904223u;uint32_t x=seed;
        rhythm_shape_t shaped={(int8_t)((x>>8)%33-16),(int8_t)((x>>14)%17-8),0,255,(x>>19)%3};
        step_t s=sequence_starter_voiced_rhythm(x%24,(x>>7)%64,(x>>13)%12,(x>>17)%3,
            (int32_t)((x>>20)%13)-6,(x>>23)%3,&shaped,(x>>26)&1,(x>>27)%18);
        hash=(hash^s.n)*16777619u;hash=(hash^s.time)*16777619u;
        hash=(hash^s.flags)*16777619u;hash=(hash^s.vel)*16777619u;
        for(uint32_t k=0;k<s.n;k++) hash=(hash^s.note[k])*16777619u;
    }
    assert(hash==0x627dceeau);
    /* Variable drafts: every duration/rhythm/mode stays within its actual phrase. */
    sequence_custom_degree[0]=4;sequence_custom_degree[1]=0;
    sequence_custom_degree[2]=1;sequence_custom_degree[3]=3;
    for(uint32_t count=1;count<=8;count++) for(uint32_t span=0;span<=2;span++) {
        sequence_custom_count=count;sequence_custom_span=span;
        uint32_t len=sequence_length(NSEQUENCE_STARTERS);
        if(len>NSTEP) continue; /* the native control prevents this combination */
        for(uint32_t mode=0;mode<3;mode++) for(uint32_t r=0;r<NSEQUENCE_RHYTHMS;r++) {
            rhythm_shape_t moved={3,-1,0,255,1};uint32_t attacks=0;
            for(uint32_t at=0;at<NSTEP;at++) {
                step_t s=sequence_starter_voiced_rhythm(NSEQUENCE_STARTERS,at,2,2,-1,mode,&moved,1,r);
                if(at>=len) assert(s.time==ST_REST&&!s.n);
                else if(s.time==ST_NOTE) {attacks++;assert(s.n==(mode==STARTER_CHORD?3:1));}
            }
            assert(attacks);
        }
    }
    sequence_custom_count=4;sequence_custom_span=2;
    /* Named rhythm attacks are independent of progression and rendering mode.
     * In particular explicit BAR must not silently acquire ARP's eighth pulse. */
    for(uint32_t r=1;r<NSEQUENCE_RHYTHMS;r++)for(g=0;g<NSEQUENCE_STARTERS;g++)
        for(mode=0;mode<3;mode++)for(i=0;i<NSTEP;i++) {
            step_t s=sequence_starter_voiced_rhythm(g,i,0,1,0,mode,&shape,1,r);
            int hit=(SEQUENCE_RHYTHMS[r].hits>>(i%16))&1;
            assert((s.time==ST_NOTE)==hit);
            assert(s.n==(hit?(mode==STARTER_CHORD?((SEQUENCE_STARTERS[g].degrees&4096u)?4:3):1):0));
            if(!hit) assert(s.time==(mode==STARTER_CHORD&&r==1?ST_TIE:ST_REST));
        }
    for(i=0;i<NSTEP;i++) {
        step_t a=sequence_starter_step_rhythm(0,i,0,1,0,STARTER_ARP,&shape,0);
        step_t b=sequence_starter_step(0,i,0,1,0,STARTER_ARP,&shape);
        assert(!memcmp(&a,&b,sizeof a));
    }
    /* Existing syncopation/rotation transform the selected mask without lost attacks. */
    for(uint32_t r=1;r<NSEQUENCE_RHYTHMS;r++) {
        rhythm_shape_t moved={3,-2,7,255,2};uint32_t hits=0;
        for(i=0;i<NSTEP;i++) hits+=sequence_starter_step_rhythm(0,i,0,1,0,STARTER_BASS,&moved,r).time==ST_NOTE;
        assert(hits==4u*(uint32_t)__builtin_popcount(SEQUENCE_RHYTHMS[r].hits));
    }
    /* Fixed musical oracle: POP FOUR keeps common tones in close voicings. */
    {
        static const uint8_t expected[4][3]={{60,64,67},{59,62,67},{60,64,69},{60,65,69}};
        for(i=0;i<4;i++) {
            step_t v=sequence_starter_voiced(0,16*i,0,1,0,STARTER_CHORD,&shape,1);
            assert(v.n==3&&!memcmp(v.note,expected[i],3));
            step_t a=sequence_starter_voiced(0,16*i,0,1,0,STARTER_ARP,&shape,1);
            assert(a.n==1&&a.note[0]==expected[i][0]);
        }
        for(g=0;g<NSEQUENCE_STARTERS;g++)for(scale=0;scale<NSCALES;scale++)
            for(mode=0;mode<3;mode++)for(i=0;i<NSTEP;i++) {
                step_t plain=sequence_starter_step(g,i,0,scale,0,mode,&shape);
                step_t off=sequence_starter_voiced(g,i,0,scale,0,mode,&shape,0);
                step_t voiced=sequence_starter_voiced(g,i,0,scale,0,mode,&shape,1);
                assert(!memcmp(&plain,&off,sizeof plain));
                if(mode==STARTER_BASS)assert(!memcmp(&plain,&voiced,sizeof plain));
                for(j=0;j<voiced.n;j++)assert(voiced.note[j]<128&&(!j||voiced.note[j]>voiced.note[j-1]));
            }
    }
    for(g=0;g<NSEQUENCE_STARTERS;g++) for(scale=0;scale<NSCALES;scale++)
        for(mode=0;mode<3;mode++) for(i=0;i<NSTEP;i++) {
            step_t s=sequence_starter_step(g,i,11,scale,3,mode,&shape);
            assert(s.n<=4 && s.time<=ST_REST);
            for(j=0;j<s.n;j++) {
                assert(s.note[j]<=127);
                if(j) assert(s.note[j]>s.note[j-1]);
            }
        }
    { step_t a=sequence_starter_step(0,0,0,1,0,0,&shape);
      step_t b=sequence_starter_step(0,16,0,1,0,0,&shape);
      step_t c=sequence_starter_step(0,32,0,1,0,0,&shape);
      assert(a.n==3&&a.note[0]==60&&a.note[1]==64&&a.note[2]==67);
      assert(b.note[0]==67&&b.note[1]==71&&b.note[2]==74);
      assert(c.note[0]==69&&c.note[1]==72&&c.note[2]==76);
      assert(sequence_starter_step(0,1,0,1,0,0,&shape).time==ST_TIE);
      assert(sequence_starter_step(0,1,0,1,0,1,&shape).time==ST_REST);
      assert(sequence_starter_step(0,0,0,1,0,1,&shape).note[0]==48);
      assert(sequence_starter_step(0,0,2,2,-1,0,&shape).note[1]==53); }
    /* Every starter must have a real chord-tone arpeggio, including sparse ones.
     * Count generated attacks and pitches rather than comparing mode flags. */
    for(g=0;g<NSEQUENCE_STARTERS;g++) {
        uint32_t bass_hits=0,arp_hits=0,tones=0;
        uint32_t degree=sequence_degree(g,0);
        for(i=0;i<16;i++) {
            step_t bass=sequence_starter_step(g,i,0,1,0,STARTER_BASS,&shape);
            step_t arp=sequence_starter_step(g,i,0,1,0,STARTER_ARP,&shape);
            if(bass.n) { bass_hits++; assert(bass.n==1&&bass.note[0]==sequence_degree_note(degree,0,1,-1)); }
            if(arp.n) {
                arp_hits++; assert(arp.n==1);
                for(j=0;j<((SEQUENCE_STARTERS[g].degrees&4096u)?4u:3u);j++)
                    if(arp.note[0]==sequence_degree_note(degree+2*j,0,1,0)) tones|=1u<<j;
            }
        }
        assert(arp_hits>=8 && arp_hits>=bass_hits);
        if(bass_hits<8) assert(arp_hits>bass_hits);
        assert(tones==((SEQUENCE_STARTERS[g].degrees&4096u)?15u:7u));
    }
    { static const uint8_t expected[]={60,64,67,60,64,67,60,64};
      for(i=0;i<8;i++) {
          step_t arp=sequence_starter_step(0,i*2,0,1,0,STARTER_ARP,&shape);
          assert(arp.time==ST_NOTE&&arp.n==1&&arp.note[0]==expected[i]);
      }
      assert(sequence_starter_step(0,16,0,1,0,STARTER_ARP,&shape).note[0]==67);
    }
    shape.rotate=3;
    for(i=0;i<NSTEP;i++) {
        step_t a=sequence_starter_step(8,i,0,1,0,0,&shape);
        step_t b=sequence_starter_step(8,(i+61)%64,0,1,0,0,0);
        assert(!memcmp(&a,&b,sizeof a));
    }
    shape.rotate=0;shape.sync=2;
    assert(sequence_starter_step(0,63,0,1,0,0,&shape).n==3);
    assert(sequence_starter_step(0,0,0,1,0,0,&shape).time==ST_TIE);
    memset(trk,0,sizeof trk); memset(&song,0,sizeof song);
    for(j=0;j<NTRK;j++) for(i=0;i<NLOCK;i++)trk[j].lock[i].step=LOCK_FREE;
    trk[0].preset=99; untouched=trk[0];song.sel=1;
    trk[1].micro[2]=-19;trk[1].fill[0]=3;trk[1].lock[0].step=2;
    assert(sequence_starter_has_content(&trk[1]));
    sequence_root=0;sequence_scale=1;sequence_sel=0;sequence_shape.feel=7;
    song.playing=1; assert(!sequence_starter_apply()&&marks==0);
    song.playing=0;transport_req=2;assert(!sequence_starter_apply()&&marks==0);
    transport_req=0;song.sel=TRK_DRUM;assert(!sequence_starter_apply()&&marks==0);
    song.sel=1;assert(sequence_starter_apply()&&marks==1&&captures==1);
    assert(!memcmp(&untouched,&trk[0],sizeof untouched));
    assert(trk[1].p[P_SLEN]==64&&trk[1].p[P_SDIV]==2&&trk[1].seq_active);
    for(i=0;i<NSTEP;i++)assert(trk[1].micro[i]==7&&!step_fill(&trk[1],i));
    for(i=0;i<NLOCK;i++)assert(trk[1].lock[i].step==LOCK_FREE);
    sequence_preview.track=1;sequence_preview.starter=0;sequence_preview.scale=1;
    sequence_preview.first=1;sequence_preview.active=1;
    sequence_preview_block(0); assert(note_ons==3&&note_offs==0);
    sequence_preview.phase=div_units(2);sequence_preview_block(0);assert(note_offs==0); /* ties retain voices */
    fm1_in.notes=1;sequence_preview_block(0);
    assert(!sequence_preview.active&&sequence_preview.n==0&&note_offs==3);
    fm1_in.notes=0;
    sequence_preview.active=sequence_preview.first=1;sequence_preview.step=0;
    sequence_preview.shape.feel=16;sequence_preview.phase=div_units(2)/4-1;
    sequence_preview_block(0);assert(note_ons==3); /* positive feel waits */
    sequence_preview.phase++;sequence_preview_block(0);assert(note_ons==6);
    sequence_preview_stop();
    sequence_preview.active=sequence_preview.first=1;sequence_preview.step=0;
    sequence_preview.shape.feel=-16;sequence_preview.phase=div_units(2)*3/4-1;
    sequence_preview_block(0);assert(note_ons==6); /* early event wraps initial lead-in */
    sequence_preview.phase++;sequence_preview_block(0);assert(note_ons==9);
    sequence_preview_stop();
    puts("sequence starters: transpose, chord quality, rhythm mapping, guards, selected-track replacement and preview ownership PASS");
    return 0;
}
