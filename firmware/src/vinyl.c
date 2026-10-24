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

static int32_t vinyl_delay(uint32_t phase, uint32_t slow, int32_t depth, unsigned mode)
{
    /* Incommensurate slow component breaks the mechanical repeating wobble.
     * Maximum excursion < 96 frames; interpolation never reads unwritten future. */
    int32_t wave = sine_i(phase) * 140 + sine_i(phase * 8u) * 36
                 + sine_i(slow) * 14;
    if (mode == VINYL_MODE_TAPE) wave = sine_i(phase)*130 + sine_i(phase*29u)*40 + sine_i(slow)*14;
    return (96 << 16) + wave * (depth >> 8) / 127;
}

static void vinyl_process_mode(int32_t *l, int32_t *r, uint32_t n, unsigned mode)
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
    int32_t depth1 = vinyl.wow + clamp(wow * 256 - vinyl.wow, -(int32_t)n * 32, (int32_t)n * 32);
    int32_t d0=vinyl_delay(vinyl.phase,vinyl.slow_phase,vinyl.wow,vinyl.mode);
    vinyl.phase += step*n; vinyl.slow_phase += (step/3u)*n;
    int32_t d1=vinyl_delay(vinyl.phase,vinyl.slow_phase,depth1,vinyl.mode);
    int32_t dd=n?(d1-d0)/(int32_t)n:0;
    uint32_t sampler_hold=0,sampler_bits=0;
    int32_t sampler_gain=256,sampler_comp=65536,sampler_rate_mix=0,sampler_crush_mix=0,sampler_drive_mix=0;
    if(mode==VINYL_MODE_SAMPLER || vinyl.mode==VINYL_MODE_SAMPLER) {
        sampler_hold=(1u<<((uint32_t)(vinyl.wow>>8)*5u/127u))-1u;
        sampler_bits=(uint32_t)fx_early_amount(vinyl.tone>>8)*12u/127u;
        /* Square the drive control: gentle lower travel, original full crunch. */
        int32_t drive=(vinyl.crackle>>8)*(vinyl.crackle>>8)/127;
        sampler_gain=256+drive*6;
        sampler_comp=16777216/sampler_gain;
        sampler_rate_mix=vinyl.wow*256/127;
        sampler_crush_mix=vinyl.tone*256/127;
        sampler_drive_mix=drive*65536/127;
    }
    for (uint32_t i=0;i<n;i++,d0+=dd) {
        vinyl.character += clamp((mode==vinyl.mode?65536:0)-vinyl.character,-128,128);
        if (!vinyl.character) vinyl.mode=mode;
        vinyl.wow += clamp(wow*256-vinyl.wow,-32,32);
        vinyl.wet += clamp((wow||vinyl.wow?32768:0)-vinyl.wet,-128,128);
        vinyl.tone += clamp(tone*256-vinyl.tone,-32,32);
        vinyl.crackle += clamp(crackle*256-vinyl.crackle,-32,32);
        vinyl.bed += clamp((song.playing?32768:0)-vinyl.bed,-8,8);
        uint32_t nz=noise32(&vinyl.rnd),w=vinyl.w++&255u;
        uint32_t pos=(w<<16)-(uint32_t)d0,a=(pos>>16)&255u;
        int32_t amount=vinyl.crackle>>8;
        /* ~43 events/s at maximum, a varied bipolar impulse with a short tail.
         * Hiss remains >40dB below the largest pop, rather than masking it. */
        if ((nz>>16)<(uint32_t)amount/2u) {
            vinyl.click=((int32_t)(nz&2047u)-1024)*amount/4;
            vinyl.damage=65536;
        }
        vinyl.click=vinyl.click*3/4;
        vinyl.damage-=clamp(vinyl.damage,0,vinyl.mode==VINYL_MODE_CD?128:32);
        int32_t hiss=((int32_t)(nz>>16)-32768)*amount;
        int32_t noise=mulq16(vinyl.click+(vinyl.mode==VINYL_MODE_RADIO?(hiss>>7):hiss/32768),vinyl.bed);
        uint32_t period=256u-(uint32_t)(vinyl.wow>>8)*192u/127u;
        if (++vinyl.cd_count>=period) { vinyl.cd_count=0; vinyl.cd_anchor=(w-32u)&255u; }
        for (uint32_t c=0;c<2;c++) {
            int32_t *out=c?r:l,dry=out[i],x=dry;
            vinyl.ring[c][w]=x;
            int32_t y=vinyl.ring[c][a]+mulq16(vinyl.ring[c][(a+1u)&255u]-vinyl.ring[c][a],pos&65535u);
            if (vinyl.mode==VINYL_MODE_SAMPLER) {
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
            } else if (vinyl.mode==VINYL_MODE_CD) {
                /* Damaged digital media: short repeated fragments, with intact
                 * input continuously captured; no content scheduling or buffers. */
                uint32_t tap=(vinyl.cd_anchor+(vinyl.cd_count&31u))&255u;
                y=vinyl.ring[c][tap];
                if (vinyl.cd_count < (period-32u)*(uint32_t)(vinyl.tone>>8)/127u)
                    x+=mulq16(y-x,amount*516u);
            } else {
                x+=mulq16(y-x,vinyl.wet*2);
                uint32_t radio=vinyl.mode==VINYL_MODE_RADIO,band=(uint32_t)clamp((vinyl.tone>>8)*4,0,127);
                vinyl.low[c]+=mulq16(x-vinyl.low[c],radio?24000-(vinyl.tone>>8)*128:(32767-(vinyl.tone>>8)*232)*2);
                if(!x && vinyl.low[c]>-32 && vinyl.low[c]<32) vinyl.low[c]=0;
                if(vinyl.tone)x=radio?x+mulq16(vinyl.low[c]-x,band*516u):vinyl.low[c];
                if(vinyl.mode==VINYL_MODE_TAPE) {
                    /* Bounded soft saturation, full bus headroom in intermediates. */
                    int32_t sat=clamp(x*2,-32768,32768);
                    sat-=mulq16(sat,sat<0?-sat:sat);
                    x+=mulq16(sat-x,amount*516u);
                    x+=noise/64;
                    if(vinyl.mode==VINYL_MODE_TAPE) {
                        x-=mulq16(x,mulq16(vinyl.damage,amount*128u));
                        x+=mulq16(hiss>>13,vinyl.bed);
                    }
                } else if(vinyl.mode==VINYL_MODE_VHS) {
                    x-=mulq16(x,mulq16(vinyl.damage,amount*384u));
                    x+=noise/128;
                } else if(vinyl.mode==VINYL_MODE_RADIO) {
                    /* Band-limited receiver: darkness narrows the band, damage adds static. */
                    vinyl.hp[c]+=mulq16(x-vinyl.hp[c],3200);
                    if(!x && vinyl.hp[c]>-32 && vinyl.hp[c]<32)vinyl.hp[c]=0;
                    x-=mulq16(vinyl.hp[c],band*516u);
                    x+=noise/8;
                } else x+=noise;
            }
            out[i]=dry+mulq16(x-dry,vinyl.character);
        }
    }
}
/* Legacy direct callers explicitly request the original VINYL character. */
static void vinyl_process(int32_t *l,int32_t *r,uint32_t n)
{ vinyl_process_mode(l,r,n,VINYL_MODE_VINYL); }
