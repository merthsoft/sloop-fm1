namespace Sloop.Core.Sampling;

public sealed class SampleDocument
{
    public PcmWave Source { get; }
    public long Start { get; private set; }
    public long End { get; private set; }
    private readonly SortedSet<long> markers = [];
    public IReadOnlyList<long> Markers => markers.ToArray();
    private readonly Stack<(long Start, long End, long[] Markers)> history = new();
    public long Revision { get; private set; }
    public bool CanUndo => history.Count > 0;
    public SampleDocument(PcmWave source) { Source = source; End = source.Frames; }
    private void Remember() { history.Push((Start, End, markers.ToArray())); Revision++; }
    public void Trim(long start, long end)
    {
        if (start < 0 || end > Source.Frames || start >= end) throw new ArgumentOutOfRangeException(nameof(start));
        if (start == Start && end == End) return;
        Remember(); Start = start; End = end;
        markers.RemoveWhere(m => m <= Start || m >= End);
    }
    public bool Split(long frame)
    {
        if (frame <= Start || frame >= End || markers.Contains(frame) || markers.Count >= 15) return false;
        Remember(); markers.Add(frame); return true;
    }
    public void EqualParts(int count)
    {
        if (count < 1 || count > 16 || End - Start < count) throw new ArgumentOutOfRangeException(nameof(count));
        Remember(); markers.Clear();
        for (int i = 1; i < count; i++) markers.Add(Start + (End - Start) * i / count);
    }
    public bool MoveBoundary(long boundary, long frame)
    {
        var bounds = new[] { Start }.Concat(markers).Append(End).ToArray();
        int index = Array.IndexOf(bounds, boundary);
        if (index <= 0 || index >= bounds.Length - 1 || frame <= bounds[index - 1] || frame >= bounds[index + 1] || frame == boundary) return false;
        Remember(); markers.Remove(boundary); markers.Add(frame); return true;
    }
    public bool RemoveBoundary(long frame)
    {
        if (!markers.Contains(frame)) return false;
        Remember(); markers.Remove(frame); return true;
    }
    /// <summary>Edit the two edges of a selected slice atomically; neighbors remain nonempty.</summary>
    public void SetSliceRange(int index, long start, long end)
    {
        var slices = Slices();
        if (index < 0 || index >= slices.Length) throw new ArgumentOutOfRangeException(nameof(index));
        long lower = index == 0 ? -1 : slices[index - 1].Start;
        long upper = index == slices.Length - 1 ? Source.Frames + 1 : slices[index + 1].End;
        if (start <= lower || end >= upper || start < 0 || end > Source.Frames || start >= end) throw new ArgumentException("Keep this slice and its neighbors nonempty.");
        if (slices[index] == (start, end)) return;
        Remember();
        if (index == 0) Start = start; else { markers.Remove(slices[index].Start); markers.Add(start); }
        if (index == slices.Length - 1) End = end; else { markers.Remove(slices[index].End); markers.Add(end); }
    }
    public void ReplaceBoundaries(IEnumerable<long> boundaries)
    {
        var values = boundaries.ToArray();
        if (values.Length > 15 || values.Any(m => m <= Start || m >= End) || !values.SequenceEqual(values.Distinct().Order()))
            throw new ArgumentException("Use up to 15 ordered, distinct boundaries inside the trim.");
        if (markers.SequenceEqual(values)) return;
        Remember(); markers.Clear(); markers.UnionWith(values);
    }
    public bool Undo()
    {
        if (!history.TryPop(out var state)) return false;
        Revision++; Start = state.Start; End = state.End; markers.Clear(); markers.UnionWith(state.Markers); return true;
    }
    public (long Start, long End)[] Slices()
    {
        long[] bounds = [Start, .. markers, End];
        return Enumerable.Range(0, bounds.Length - 1).Select(i => (bounds[i], bounds[i + 1])).ToArray();
    }
    public void Save(string path)
    {
        var temp = path + ".tmp";
        File.WriteAllLines(temp, new[] { "SLOOP-SAMPLE-1", Start.ToString(System.Globalization.CultureInfo.InvariantCulture),
            End.ToString(System.Globalization.CultureInfo.InvariantCulture) }
            .Concat(markers.Select(m => m.ToString(System.Globalization.CultureInfo.InvariantCulture))));
        File.Move(temp, path, true);
    }
    public static SampleDocument Restore(PcmWave source, string path)
    {
        var lines = File.ReadAllLines(path);
        if (lines.Length < 3 || lines.Length > 18 || lines[0] != "SLOOP-SAMPLE-1") throw new FormatException("Invalid sample edit document.");
        var doc = new SampleDocument(source);
        try
        {
            doc.Trim(long.Parse(lines[1]), long.Parse(lines[2]));
            foreach (var line in lines.Skip(3)) if (!doc.Split(long.Parse(line))) throw new FormatException("Invalid marker.");
        }
        catch (ArgumentOutOfRangeException error) { throw new FormatException("Invalid trim.", error); }
        doc.history.Clear(); return doc;
    }
}
