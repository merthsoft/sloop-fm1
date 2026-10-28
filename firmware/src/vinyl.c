/* SPDX-License-Identifier: GPL-3.0-only */
/* Master LOFI. The existing 256-frame stereo history is shared by every mode. */
static int punch_vinyl_requested(void);
enum { VINYL_MODE_VINYL, VINYL_MODE_TAPE, VINYL_MODE_VHS, VINYL_MODE_CD, VINYL_MODE_OFF, VINYL_MODE_RADIO, VINYL_MODE_SAMPLER, VINYL_MODE_COUNT };
static struct {
    int32_t ring[2][256], low[2], hp[2], wet, wow, tone, crackle, bed, click, rnd;
    uint32_t w, phase, slow_phase, cd_count, cd_anchor;
    int32_t character, damage, punch_gain;
    unsigned mode;
} vinyl;

/* Exact truncation toward zero; -Oz otherwise uses hardware division even for
 * these power-of-two constants in the per-sample loop. Bias only negatives. */
DSP_INLINE int32_t vinyl_div_pow2(int32_t v,uint32_t shift)
{
    return (v+(v<0?(int32_t)((1u<<shift)-1u):0))>>shift;
}
/* ceil(2^23/127), exact for the proven 0..32767 control-product range. */
DSP_INLINE uint32_t vinyl_div127(uint32_t v) {return (v*66053u)>>23;}

static int32_t vinyl_delay(uint32_t phase, uint32_t slow, int32_t depth, unsigned mode)
{
    /* Incommensurate slow component breaks the mechanical repeating wobble.
     * Maximum excursion < 96 frames; interpolation never reads unwritten future. */
    int32_t wave = sine_i(phase) * 140 + sine_i(phase * 8u) * 36
                 + sine_i(slow) * 14;
    if (mode == VINYL_MODE_TAPE) wave = sine_i(phase)*130 + sine_i(phase*29u)*40 + sine_i(slow)*14;
    return (96 << 16) + wave * (depth >> 8) / 127;
}

/* Keep this large loop out of mix_block's already crowded register lifetime. */
static __attribute__((noinline)) void vinyl_process_mode(int32_t *l, int32_t *r, uint32_t n, unsigned mode)
{
    int32_t wow = song.g[G_VWOW], tone = song.g[G_VTONE], crackle = song.g[G_VCRACKLE];
    /* Punch boosts the selected character without rewriting the saved controls.
     * OFF uses VINYL temporarily; amounts above the punch floors stay intact. */
    int punched = punch_vinyl_requested();
    int32_t boosted_wow = wow < 96 ? 96 : wow;
    int32_t boosted_tone = tone < 90 ? 90 : tone;
    int32_t boosted_crackle = crackle < 100 ? 100 : crackle;
    /* At zero the existing target slews already provide anti-click smoothing;
     * retain their original character exactly. Additional fade is opt-in. */
    if (!fxfade) vinyl.punch_gain = punched ? 32768 : 0;
    else {
        int32_t step = (int32_t)n * punch_fade_step();
        vinyl.punch_gain += clamp((punched ? 32768 : 0) - vinyl.punch_gain, -step, step);
    }
    if (mode == VINYL_MODE_OFF) wow=tone=crackle=0;
    wow += (boosted_wow-wow)*vinyl.punch_gain/32768;
    tone += (boosted_tone-tone)*vinyl.punch_gain/32768;
    crackle += (boosted_crackle-crackle)*vinyl.punch_gain/32768;
    if(mode==VINYL_MODE_RADIO)wow=fx_early_amount(wow);
    if (mode == VINYL_MODE_OFF || mode >= VINYL_MODE_COUNT) mode = VINYL_MODE_VINYL;
    if (!(wow | tone | crackle | vinyl.wet | vinyl.wow | vinyl.tone | vinyl.crackle)) {
        if (!song.playing) vinyl.bed = 0; /* STOP may have happened while fully bypassed. */
        if (vinyl.w) {
            memset(vinyl.ring, 0, sizeof vinyl.ring);
            vinyl.w = 0; vinyl.low[0] = vinyl.low[1] = vinyl.hp[0] = vinyl.hp[1] = 0;
        }
        vinyl.click=vinyl.damage=0; vinyl.mode=mode; vinyl.character=0;
        return;
    }
    if (!vinyl.rnd) vinyl.rnd = 0x3156494E;
    uint32_t step = (uint32_t)(0x100000000ull * 7u / (9u * FS));
    if (vinyl.mode == VINYL_MODE_VHS) step *= 3;
    if (vinyl.mode == VINYL_MODE_TAPE) step /= 2;
    uint32_t phase=vinyl.phase,slow=vinyl.slow_phase;
    vinyl.phase += step*n; vinyl.slow_phase += (step/3u)*n;
    int delay_needed=(wow || vinyl.wow || vinyl.wet) &&
        (mode!=vinyl.mode || (mode!=VINYL_MODE_CD && mode!=VINYL_MODE_SAMPLER));
    int32_t d0=0,dd=0;
    if(delay_needed) {
        int32_t depth1=vinyl.wow+clamp(wow*256-vinyl.wow,-(int32_t)n*32,(int32_t)n*32);
        d0=vinyl_delay(phase,slow,vinyl.wow,vinyl.mode);
        int32_t d1=vinyl_delay(vinyl.phase,vinyl.slow_phase,depth1,vinyl.mode);
        dd=n==CTL?vinyl_div_pow2(d1-d0,CTL_LOG2):n?(d1-d0)/(int32_t)n:0;
    }
    uint32_t sampler_hold=0,sampler_bits=0;
    int32_t sampler_gain=256,sampler_comp=65536,sampler_rate_mix=0,sampler_crush_mix=0,sampler_drive_mix=0;
    if(mode==VINYL_MODE_SAMPLER || vinyl.mode==VINYL_MODE_SAMPLER) {
        sampler_hold=(1u<<vinyl_div127((uint32_t)(vinyl.wow>>8)*5u))-1u;
        sampler_bits=vinyl_div127((uint32_t)fx_early_amount(vinyl.tone>>8)*12u);
        /* Square the drive control: gentle lower travel, original full crunch. */
        int32_t drive=(int32_t)vinyl_div127((vinyl.crackle>>8)*(vinyl.crackle>>8));
        sampler_gain=256+drive*6;
        sampler_comp=16777216/sampler_gain;
        sampler_rate_mix=vinyl.wow*256/127;
        sampler_crush_mix=vinyl.tone*256/127;
        sampler_drive_mix=drive*65536/127;
    }
    /* Most blocks have settled controls. Keep the exact per-sample slew path
     * for knob/punch/mode/transport changes, without paying for it at rest. */
    int steady=!((mode^vinyl.mode) | (vinyl.character^65536) |
        (vinyl.wow^(wow*256)) | (vinyl.tone^(tone*256)) | (vinyl.crackle^(crackle*256)) |
        (vinyl.wet^(wow?32768:0)) | (vinyl.bed^(song.playing?32768:0)));
    uint32_t period=0,radio=0,band=0,low_k=0,cd_window=0;
    for (uint32_t i=0;i<n;i++,d0+=dd) {
        if(!steady) {
            vinyl.character += clamp((mode==vinyl.mode?65536:0)-vinyl.character,-128,128);
            if (!vinyl.character) vinyl.mode=mode;
            vinyl.wow += clamp(wow*256-vinyl.wow,-32,32);
            vinyl.wet += clamp((wow||vinyl.wow?32768:0)-vinyl.wet,-128,128);
            vinyl.tone += clamp(tone*256-vinyl.tone,-32,32);
            vinyl.crackle += clamp(crackle*256-vinyl.crackle,-32,32);
            vinyl.bed += clamp((song.playing?32768:0)-vinyl.bed,-8,8);
        }
        if(!steady || !i) {
            period=256u-vinyl_div127((uint32_t)(vinyl.wow>>8)*192u);
            cd_window=vinyl.mode==VINYL_MODE_CD?
                vinyl_div127((period-32u)*(uint32_t)(vinyl.tone>>8)):0;
            radio=vinyl.mode==VINYL_MODE_RADIO;
            band=(uint32_t)clamp((vinyl.tone>>8)*4,0,127);
            low_k=radio?24000-(vinyl.tone>>8)*128:(32767-(vinyl.tone>>8)*232)*2;
        }
        unsigned active=vinyl.mode;
        int32_t bed=vinyl.bed,wet=vinyl.wet*2,character=vinyl.character;
        uint32_t nz=noise32(&vinyl.rnd),w=vinyl.w++&255u;
        uint32_t pos=(w<<16)-(uint32_t)d0,a=(pos>>16)&255u;
        int32_t amount=vinyl.crackle>>8;
        /* ~43 events/s at maximum, a varied bipolar impulse with a short tail.
         * Hiss remains >40dB below the largest pop, rather than masking it. */
        if ((nz>>16)<(uint32_t)amount/2u) {
            vinyl.click=vinyl_div_pow2(((int32_t)(nz&2047u)-1024)*amount,2);
            vinyl.damage=65536;
        }
        vinyl.click=vinyl_div_pow2(vinyl.click*3,2);
        vinyl.damage-=clamp(vinyl.damage,0,active==VINYL_MODE_CD?128:32);
        int32_t hiss=((int32_t)(nz>>16)-32768)*amount;
        int32_t noise=0;
        if(active!=VINYL_MODE_CD && active!=VINYL_MODE_SAMPLER)
            noise=mulq16(vinyl.click+(radio?(hiss>>7):vinyl_div_pow2(hiss,15)),bed);
        if (++vinyl.cd_count>=period) { vinyl.cd_count=0; vinyl.cd_anchor=(w-32u)&255u; }
        /* Stereo channels share these controls, including during their slews.
         * Compute once per frame; channel filter histories remain separate. */
        uint32_t dropout=active==VINYL_MODE_TAPE?mulq16(vinyl.damage,amount*128u):
                         active==VINYL_MODE_VHS?mulq16(vinyl.damage,amount*384u):0;
        for (uint32_t c=0;c<2;c++) {
            int32_t *out=c?r:l,dry=out[i],x=dry;
            vinyl.ring[c][w]=x;
            if (active==VINYL_MODE_SAMPLER) {
                /* ADC-style rate reduction reads held samples from the same
                 * history; symmetric truncation avoids a quantizer DC offset. */
                int32_t z=dry+mulq16(vinyl.ring[c][w&~sampler_hold]-dry,sampler_rate_mix);
                /* The bus has headroom beyond 16-bit PCM. Never hard-clip it
                 * merely by selecting SAMPLER or moving an unrelated knob. */
                /* mulq16 accepts a fractional coefficient, not gain above one. */
                int32_t gained=z*(sampler_gain>>8)+mulq16(z,(sampler_gain&255)*256);
                int32_t driven=mulq16(clamp(gained,-131072,131072),sampler_comp);
                z+=mulq16(driven-z,sampler_drive_mix);
                int32_t q=((z<0?-z:z)>>sampler_bits)<<sampler_bits;
                x=z+mulq16((z<0?-q:q)-z,sampler_crush_mix);
            } else if (active==VINYL_MODE_CD) {
                /* Damaged digital media: short repeated fragments, with intact
                 * input continuously captured; no content scheduling or buffers. */
                uint32_t tap=(vinyl.cd_anchor+(vinyl.cd_count&31u))&255u;
                int32_t y=vinyl.ring[c][tap];
                if (vinyl.cd_count < cd_window)
                    x+=mulq16(y-x,amount*516u);
            } else {
                if(delay_needed) {
                    int32_t y=vinyl.ring[c][a]+mulq16(vinyl.ring[c][(a+1u)&255u]-vinyl.ring[c][a],pos&65535u);
                    x+=mulq16(y-x,wet);
                }
                vinyl.low[c]+=mulq16(x-vinyl.low[c],low_k);
                if(!x && vinyl.low[c]>-32 && vinyl.low[c]<32) vinyl.low[c]=0;
                if(vinyl.tone)x=radio?x+mulq16(vinyl.low[c]-x,band*516u):vinyl.low[c];
                if(active==VINYL_MODE_TAPE) {
                    /* Bounded soft saturation, full bus headroom in intermediates. */
                    int32_t sat=clamp(x*2,-32768,32768);
                    sat-=mulq16(sat,sat<0?-sat:sat);
                    x+=mulq16(sat-x,amount*516u);
                    x+=vinyl_div_pow2(noise,6);
                    x-=mulq16(x,dropout);
                    x+=mulq16(hiss>>13,bed);
                } else if(active==VINYL_MODE_VHS) {
                    x-=mulq16(x,dropout);
                    x+=vinyl_div_pow2(noise,7);
                } else if(active==VINYL_MODE_RADIO) {
                    /* Band-limited receiver: darkness narrows the band, damage adds static. */
                    vinyl.hp[c]+=mulq16(x-vinyl.hp[c],3200);
                    if(!x && vinyl.hp[c]>-32 && vinyl.hp[c]<32)vinyl.hp[c]=0;
                    x-=mulq16(vinyl.hp[c],band*516u);
                    x+=vinyl_div_pow2(noise,3);
                } else x+=noise;
            }
            out[i]=dry+mulq16(x-dry,character);
        }
    }
}
/* Legacy direct callers explicitly request the original VINYL character. */
static void vinyl_process(int32_t *l,int32_t *r,uint32_t n)
{ vinyl_process_mode(l,r,n,VINYL_MODE_VINYL); }
