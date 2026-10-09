// SPDX-License-Identifier: GPL-3.0-only
namespace Sloop.SampleEncoding;

/// <summary>PCM and loop coordinates are relative to an already extracted immutable source slice.</summary>
public sealed record SourceZone(ReadOnlyMemory<double> InterleavedPcm, int Channels, int SourceRate,
    ConversionOptions Options, int RootNote, int? LowNote = null, int? HighNote = null,
    int? LoopStartFrame = null, int? LoopEndFrameExclusive = null);
public sealed record ConvertedZone(ZoneInput Zone, ConversionReport Report);
public sealed record ConvertedKit(SlotArtifact Artifact, IReadOnlyList<ConversionReport> Reports);

public static class ConversionPipeline
{
    public static ConvertedZone ConvertZone(SourceZone source, CancellationToken token=default)
    {
        ArgumentNullException.ThrowIfNull(source);
        var pcm = PcmConversion.Convert(source.InterleavedPcm.Span, source.Channels, source.SourceRate, source.Options, token);
        int? start = null, end = null;
        if (source.LoopStartFrame.HasValue != source.LoopEndFrameExclusive.HasValue || source.LoopStartFrame < 0 || source.LoopEndFrameExclusive > pcm.Report.SourceFrames || source.LoopStartFrame >= source.LoopEndFrameExclusive)
            throw new ArgumentException("Invalid source loop frame range.");
        if (source.LoopStartFrame is int ls)
        {
            start = PcmConversion.MapBoundary(ls, source.SourceRate);
            end = PcmConversion.MapBoundary(source.LoopEndFrameExclusive!.Value, source.SourceRate);
            if (start >= end) throw new ArgumentException("Loop collapsed during resampling; choose a longer loop.");
        }
        return new(new(pcm.Samples, source.RootNote, source.LowNote, source.HighNote, start, end), pcm.Report);
    }
    public static ConvertedKit Build(string name, IReadOnlyList<SourceZone> sources, CancellationToken token=default)
    {
        ArgumentNullException.ThrowIfNull(sources);
        if (sources.Count is < 1 or > 16) throw new ArgumentException("1..16 source zones required.");
        var converted = sources.Select(s=>ConvertZone(s,token)).ToArray();
        token.ThrowIfCancellationRequested();
        return new(SlotBuilder.Build(name, converted.Select(z => z.Zone).ToArray()), Array.AsReadOnly(converted.Select(z => z.Report).ToArray()));
    }
}
