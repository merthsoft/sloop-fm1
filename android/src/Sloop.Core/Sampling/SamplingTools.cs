namespace Sloop.Core.Sampling;

/// <summary>Frame-based viewport independent of view pixels and edit history.</summary>
public sealed class SampleViewport(long frames)
{
    public long Start { get; private set; }
    public long End { get; private set; } = frames > 0 ? frames : throw new ArgumentOutOfRangeException(nameof(frames));
    public long Length => End - Start;
    public long FrameAt(double fraction) => Math.Clamp(Start + (long)(Math.Clamp(fraction, 0, 1) * Length), Start, End - 1);
    public void Show(long start, long end)
    {
        if (start < 0 || end > frames || start >= end) throw new ArgumentOutOfRangeException(nameof(start));
        Start = start; End = end;
    }
    public void Zoom(double factor, long anchor)
    {
        if (!double.IsFinite(factor) || factor <= 0) throw new ArgumentOutOfRangeException(nameof(factor));
        var length = (long)Math.Clamp(Length / factor, 1, frames);
        var fraction = Math.Clamp((anchor - Start) / (double)Length, 0, 1);
        Start = Math.Clamp(anchor - (long)(length * fraction), 0, frames - length); End = Start + length;
    }
    public void Pan(long delta) { var length = Length; Start = Math.Clamp(Start + Math.Clamp(delta, -frames, frames), 0, frames - length); End = Start + length; }
}

public sealed record TransientProposal(long Revision, long Start, long End, IReadOnlyList<long> Boundaries);

public static class SamplingTools
{
    /// <summary>Streaming 2ms peak envelope, rising-energy onsets; fixed memory, at most 15 chops.</summary>
    public static TransientProposal Detect(PcmWave source, long start, long end, long revision,
        double sensitivity = .5, int spacingMilliseconds = 80, CancellationToken token = default)
    {
        if (start < 0 || end > source.Frames || start >= end || !double.IsFinite(sensitivity) || sensitivity is < 0 or > 1 || spacingMilliseconds is < 1 or > 2000)
            throw new ArgumentOutOfRangeException(nameof(start));
        int window = Math.Max(1, source.SampleRate / 500);
        long spacing = Math.Max(1, (long)source.SampleRate * spacingMilliseconds / 1000);
        var candidates = new List<(long Frame, double Strength)>();
        using var stream = File.OpenRead(source.Path);
        stream.Position = source.DataOffset + start * source.Channels * 2;
        using var reader = new BinaryReader(stream);
        double baseline = 0, previous = 0;
        long last = start;
        for (long frame = start; frame < end; frame += window)
        {
            token.ThrowIfCancellationRequested();
            double peak = 0;
            long stop = Math.Min(end, frame + window);
            for (long f = frame; f < stop; f++)
                for (int channel = 0; channel < source.Channels; channel++)
                    peak = Math.Max(peak, Math.Abs((int)reader.ReadInt16()) / 32768.0);
            double rise = peak - previous;
            double threshold = Math.Max(.015 + (1 - sensitivity) * .12, baseline * (1.2 + (1 - sensitivity) * 3));
            if (rise > threshold && frame - last >= spacing && end - frame >= spacing)
            {
                candidates.Add((frame, rise)); last = frame;
                // Retain the strongest candidates without growing memory with source duration.
                if (candidates.Count > 15) candidates.RemoveAt(candidates.FindIndex(c => c.Strength == candidates.Min(v => v.Strength)));
            }
            baseline = baseline * .94 + peak * .06; previous = peak;
        }
        return new(revision, start, end, Array.AsReadOnly(candidates.Select(c => c.Frame).Order().ToArray()));
    }

    public static float[] Peaks(PcmWave source, long start, long end, int count = 512, CancellationToken token = default)
    {
        if (start < 0 || end > source.Frames || start >= end || count is < 1 or > 4096) throw new ArgumentOutOfRangeException(nameof(start));
        float[] peaks = new float[count];
        using var stream = File.OpenRead(source.Path);
        stream.Position = source.DataOffset + start * source.Channels * 2;
        using var reader = new BinaryReader(stream);
        for (long frame = start; frame < end; frame++)
        {
            if ((frame & 4095) == 0) token.ThrowIfCancellationRequested();
            int bucket = (int)((frame - start) * count / (end - start));
            for (int channel = 0; channel < source.Channels; channel++) peaks[bucket] = Math.Max(peaks[bucket], Math.Abs((int)reader.ReadInt16()) / 32768f);
        }
        return peaks;
    }
}
