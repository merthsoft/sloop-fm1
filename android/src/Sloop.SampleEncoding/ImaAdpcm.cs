// SPDX-License-Identifier: GPL-3.0-only
// Adapted from tools/sampleio.py and firmware/src/eng_sample.c.
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
namespace Sloop.SampleEncoding;

public readonly record struct AdpcmState(short Predictor, byte Index);
public sealed record AdpcmResult(byte[] Data, AdpcmState LoopState);

public static class ImaAdpcm
{
    private static readonly int[] Steps = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767];
    private static readonly int[] Adjust = [-1,-1,-1,-1,2,4,6,8];
    public static AdpcmResult Encode(ReadOnlySpan<short> pcm, int loopStart = 0)
    {
        if (pcm.IsEmpty || loopStart < 0 || loopStart >= pcm.Length) throw new ArgumentOutOfRangeException(nameof(loopStart));
        var data = new byte[(pcm.Length / 2) + (pcm.Length & 1)];
        int pred = 0, index = 0;
        AdpcmState state = default;
        for (int n = 0; n < pcm.Length; n++)
        {
            if (n == loopStart) state = new((short)pred, (byte)index);
            int step = Steps[index], diff = pcm[n] - pred, code = 0;
            if (diff < 0) { code = 8; diff = -diff; }
            if (diff >= step) { code |= 4; diff -= step; }
            if (diff >= (step >> 1)) { code |= 2; diff -= step >> 1; }
            if (diff >= (step >> 2)) code |= 1;
            Advance(code, ref pred, ref index);
            data[n / 2] |= (byte)(code << ((n & 1) * 4));
        }
        return new(data, state);
    }
    public static short[] Decode(ReadOnlySpan<byte> data, int sampleCount, AdpcmState initial = default, int startSample = 0)
    {
        if (sampleCount < 0 || startSample < 0 || (long)startSample + sampleCount > (long)data.Length * 2 || initial.Index > 88) throw new ArgumentOutOfRangeException(nameof(sampleCount));
        int pred = initial.Predictor, index = initial.Index;
        var pcm = new short[sampleCount];
        for (int n = 0; n < sampleCount; n++)
        {
            int pos = startSample + n;
            Advance((data[pos / 2] >> ((pos & 1) * 4)) & 15, ref pred, ref index);
            pcm[n] = (short)pred;
        }
        return pcm;
    }
    private static void Advance(int code, ref int pred, ref int index)
    {
        int step = Steps[index], delta = step >> 3;
        if ((code & 4) != 0) delta += step;
        if ((code & 2) != 0) delta += step >> 1;
        if ((code & 1) != 0) delta += step >> 2;
        pred = Math.Clamp(pred + ((code & 8) != 0 ? -delta : delta), -32768, 32767);
        index = Math.Clamp(index + Adjust[code & 7], 0, 88);
    }
}
