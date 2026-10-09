namespace Sloop.Core.Sampling;

/// <summary>A review draft tied to an unchanged source document, never a wall-clock timer.</summary>
public sealed class TapChopReview(SampleDocument document)
{
    private readonly long revision = document.Revision;
    private readonly List<long> boundaries = [];
    public bool IsCurrent(SampleDocument? current) => ReferenceEquals(document, current) && current.Revision == revision;
    public IReadOnlyList<long> Boundaries => boundaries.Order().ToArray();
    public bool AddPlaybackPosition(long previewStart, long previewEnd, int positionMilliseconds, int latencyMilliseconds = 0)
    {
        if (!IsCurrent(document) || previewStart < document.Start || previewEnd > document.End || previewStart >= previewEnd || positionMilliseconds < 0 || latencyMilliseconds is < 0 or > 1000) return false;
        long frame = previewStart + (long)Math.Max(0, positionMilliseconds - latencyMilliseconds) * document.Source.SampleRate / 1000;
        if (frame <= previewStart || frame >= previewEnd || boundaries.Contains(frame) || boundaries.Count == 15) return false;
        boundaries.Add(frame); return true;
    }
    public bool Undo() { if (boundaries.Count == 0) return false; boundaries.RemoveAt(boundaries.Count - 1); return true; }
    public void Apply(SampleDocument current)
    {
        if (!IsCurrent(current)) throw new InvalidOperationException("Sample changed; start a new tap review.");
        current.ReplaceBoundaries(Boundaries);
    }
}
