// SPDX-License-Identifier: GPL-3.0-only
namespace Sloop.SampleEncoding;

public enum MonoChoice { AverageChannels, FirstChannel, SelectedChannel }
public sealed record ConversionOptions(MonoChoice Mono, double Gain, int SelectedChannel = 0);
public sealed record ConversionReport(int SourceFrames, int OutputSamples, int ClippedSamples, int CancellationFrames, double PeakBeforeQuantization);
public sealed record ConvertedPcm(short[] Samples, ConversionReport Report);

/// <summary>Pure derived conversion; caller retains original recording and edit document.</summary>
public static class PcmConversion
{
    public const int TargetRate = 22050;
    public static ConvertedPcm Convert(ReadOnlySpan<double> interleaved, int channels, int sourceRate, ConversionOptions options, CancellationToken token=default)
    {
        ArgumentNullException.ThrowIfNull(options);
        if (channels < 1 || channels > 64 || sourceRate < 1 || sourceRate > 768000 || interleaved.IsEmpty || interleaved.Length % channels != 0) throw new ArgumentException("Complete nonempty PCM frames and a valid rate/channel count required.");
        if (!Enum.IsDefined(options.Mono) || !double.IsFinite(options.Gain) || options.Gain < 0 || options.SelectedChannel < 0 || options.SelectedChannel >= channels) throw new ArgumentException("Explicit valid downmix and finite nonnegative gain required.");
        int frames = interleaved.Length / channels, cancellation = 0;
        var mono = new double[frames];
        for (int f = 0; f < frames; f++)
        {
            if ((f & 4095)==0) token.ThrowIfCancellationRequested();
            double sum = 0, magnitude = 0;
            for (int c = 0; c < channels; c++)
            {
                double v = interleaved[f * channels + c];
                if (!double.IsFinite(v) || v < -1 || v > 1) throw new ArgumentException("PCM values must be finite and in [-1,1].");
                sum += v; magnitude += Math.Abs(v);
            }
            if (channels > 1 && magnitude / channels > 0.01 && Math.Abs(sum) < magnitude * 0.1) cancellation++;
            mono[f] = (options.Mono == MonoChoice.AverageChannels ? sum / channels : interleaved[f * channels + (options.Mono == MonoChoice.FirstChannel ? 0 : options.SelectedChannel)]) * options.Gain;
            if (!double.IsFinite(mono[f])) throw new ArgumentException("Gain overflow.");
        }
        var converted = Resample(mono, sourceRate, token);
        int clipped = 0; double peak = 0;
        var pcm = new short[converted.Length];
        for (int i = 0; i < pcm.Length; i++)
        {
            double value = converted[i];
            if (!double.IsFinite(value)) throw new ArgumentException("Gain overflow during resampling.");
            peak = Math.Max(peak, Math.Abs(value));
            if (value < -1 || value > 32767.0 / 32768) clipped++;
            pcm[i] = (short)Math.Clamp(Math.Truncate(value * 32768), -32768, 32767);
        }
        return new(pcm, new(frames, pcm.Length, clipped, cancellation, peak));
    }
    // 32 zero crossings per side, Blackman-windowed sinc. Scales support for downsampling.
    // Edge extension and normalized kernel preserve DC. Same-rate conversion is exact.
    private static double[] Resample(double[] source, int rate, CancellationToken token)
    {
        if (rate == TargetRate) return (double[])source.Clone();
        int count = checked((int)(((long)source.Length * TargetRate + rate - 1) / rate));
        var result = new double[count];
        double cutoff = Math.Min(1.0, (double)TargetRate / rate), radius = 32 / cutoff;
        for (int n = 0; n < count; n++)
        {
            if ((n & 255)==0) token.ThrowIfCancellationRequested();
            double position = (double)n * rate / TargetRate, sum = 0, weightSum = 0;
            int first = (int)Math.Ceiling(position - radius), last = (int)Math.Floor(position + radius);
            for (int i = first; i <= last; i++)
            {
                double distance = i - position, t = distance / radius, x = Math.PI * distance * cutoff;
                double weight = (x == 0 ? 1 : Math.Sin(x) / x) * (0.42 + 0.5 * Math.Cos(Math.PI * t) + 0.08 * Math.Cos(2 * Math.PI * t));
                sum += source[Math.Clamp(i, 0, source.Length - 1)] * weight; weightSum += weight;
            }
            result[n] = sum / weightSum;
        }
        return result;
    }
    /// <summary>Map a half-open source boundary to converted frames, rounding upward.</summary>
    public static int MapBoundary(int frame, int sourceRate)
    {
        if (frame < 0 || sourceRate < 1 || sourceRate > 768000) throw new ArgumentOutOfRangeException(nameof(frame));
        return checked((int)(((long)frame * TargetRate + sourceRate - 1) / sourceRate));
    }
}
