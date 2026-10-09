// SPDX-License-Identifier: GPL-3.0-only
// Layout adapted from tools/sampleio.py, Copyright (C) 2026 Leo Kuroshita
// (@kurogedelic), Hügelton Instruments.
using System.Buffers.Binary;
using System.Text;
namespace Sloop.SampleEncoding;

/// <summary>Loop boundaries are half-open in encoded 22050-Hz PCM. Wire loop end is inclusive.</summary>
public sealed record ZoneInput(ReadOnlyMemory<short> Pcm, int RootNote, int? LowNote = null, int? HighNote = null, int? LoopStart = null, int? LoopEndExclusive = null);
public sealed record ZonePreview(int InputIndex, int RootNote, int LowNote, int HighNote, int DataOffset, short[] DecodedPcm);
public sealed record FitReport(long Samples, long DataBytes, int CapacityBytes)
{
    public bool Fits => DataBytes <= CapacityBytes;
    public long RemainingBytes => CapacityBytes - DataBytes;
    public double DurationSeconds => Samples / 22050.0;
    public long Pcm16Bytes => checked(Samples * 2);
    public int ArtifactBytes => checked(512 + (int)DataBytes);
}
public sealed record SlotArtifact(byte[] Header, byte[] Data, byte[] Image, FitReport Fit, IReadOnlyList<ZonePreview> Preview);

public static class SlotBuilder
{
    public const int SlotBytes = 81920, HeaderBytes = 480, DataOffset = 512, Capacity = SlotBytes - DataOffset;
    private static readonly int[] LaneNotes = [36,35,38,39,42,46,44,37,40,43,48,49,51,70,63,56];
    public static int DrumLaneNote(int lane) => lane is >= 0 and < 16 ? LaneNotes[lane] : throw new ArgumentOutOfRangeException(nameof(lane));
    public static ZoneInput DrumZone(ReadOnlyMemory<short> pcm, int lane) => new(pcm, DrumLaneNote(lane), DrumLaneNote(lane), DrumLaneNote(lane));
    public static ZoneInput ChopZone(ReadOnlyMemory<short> pcm, int midiNote) => new(pcm, midiNote, midiNote, midiNote);
    public static FitReport Measure(IEnumerable<int> sampleCounts)
    {
        ArgumentNullException.ThrowIfNull(sampleCounts);
        long samples = 0, bytes = 0; int zones = 0;
        foreach (int n in sampleCounts) { if (n < 1) throw new ArgumentException("Empty zone."); zones++; samples = checked(samples + n); bytes = checked(bytes + n / 2 + (n & 1)); }
        if (zones is < 1 or > 16) throw new ArgumentException("1..16 zones required.");
        return new(samples, bytes, Capacity);
    }
    public static SlotArtifact Build(string name, IReadOnlyList<ZoneInput> inputs)
    {
        ArgumentNullException.ThrowIfNull(name); ArgumentNullException.ThrowIfNull(inputs);
        // Snapshot all caller memory before assembly; no edits or normalization.
        var zones = inputs.Select((z, i) => (Zone: z with { Pcm = z.Pcm.ToArray() }, Index: i)).ToArray();
        var fit = Measure(zones.Select(z => z.Zone.Pcm.Length));
        foreach (var entry in zones) Validate(entry.Zone);
        if (!fit.Fits) throw new ArgumentException($"ADPCM requires {fit.DataBytes} bytes; capacity is {Capacity} bytes.");
        var data = new byte[(int)fit.DataBytes]; var header = new byte[HeaderBytes];
        var offsets = new int[zones.Length]; var states = new AdpcmState[zones.Length];
        int offset = 0;
        foreach (var entry in zones)
        {
            offsets[entry.Index] = offset;
            var encoded = ImaAdpcm.Encode(entry.Zone.Pcm.Span, entry.Zone.LoopStart ?? 0);
            states[entry.Index] = encoded.LoopState; encoded.Data.CopyTo(data, offset); offset += encoded.Data.Length;
        }
        Write32(header, 0, 0x504D5346); BinaryPrimitives.WriteUInt16LittleEndian(header.AsSpan(4), 1); header[6] = (byte)zones.Length;
        // Unicode uppercase first, then ASCII filtering, matching Python (including sharp-s expansion).
        string upper = name.Replace("ß", "SS").Replace("ﬀ", "FF").Replace("ﬁ", "FI").Replace("ﬂ", "FL").Replace("ﬃ", "FFI").Replace("ﬄ", "FFL").Replace("ﬅ", "ST").Replace("ﬆ", "ST").ToUpperInvariant();
        Encoding.ASCII.GetBytes(new string(upper.Where(c => c is >= ' ' and <= '~').Take(8).ToArray())).CopyTo(header, 8);
        Write32(header, 16, (uint)data.Length); Write32(header, 20, Crc32(data));
        var sorted = zones.OrderBy(z => z.Zone.RootNote).ToArray(); var previews = new List<ZonePreview>();
        for (int j = 0; j < sorted.Length; j++)
        {
            var (z, index) = sorted[j]; int p = 32 + j * 28;
            int lo = z.LowNote ?? (j == 0 ? 0 : (sorted[j - 1].Zone.RootNote + z.RootNote) / 2 + 1);
            int hi = z.HighNote ?? (j == sorted.Length - 1 ? 127 : (z.RootNote + sorted[j + 1].Zone.RootNote) / 2);
            if (lo > hi) throw new ArgumentException("Automatic key split produced an empty range; use distinct roots or explicit ranges.");
            Write32(header, p, (uint)offsets[index]); Write32(header, p + 4, (uint)z.Pcm.Length);
            Write32(header, p + 8, (uint)(z.LoopStart ?? 0)); Write32(header, p + 12, (uint)((z.LoopEndExclusive ?? z.Pcm.Length) - 1)); Write32(header, p + 16, 32768);
            BinaryPrimitives.WriteInt16LittleEndian(header.AsSpan(p + 20), (short)(z.RootNote * 16)); BinaryPrimitives.WriteInt16LittleEndian(header.AsSpan(p + 22), states[index].Predictor);
            header[p + 24] = states[index].Index; header[p + 25] = (byte)lo; header[p + 26] = (byte)hi; header[p + 27] = (byte)(z.LoopStart.HasValue ? 1 : 0);
            previews.Add(new(index, z.RootNote, lo, hi, offsets[index], ImaAdpcm.Decode(data.AsSpan(offsets[index]), z.Pcm.Length)));
        }
        var image = new byte[DataOffset + data.Length]; header.CopyTo(image, 0); data.CopyTo(image, DataOffset);
        return new(header, data, image, fit, previews.AsReadOnly());
    }
    private static void Validate(ZoneInput z)
    {
        if (z.RootNote is < 0 or > 127 || z.LowNote.HasValue != z.HighNote.HasValue || z.LowNote is < 0 or > 127 || z.HighNote is < 0 or > 127 || z.LowNote > z.HighNote) throw new ArgumentException("Invalid MIDI root/key range.");
        if (z.LoopStart.HasValue != z.LoopEndExclusive.HasValue || z.LoopStart < 0 || z.LoopEndExclusive > z.Pcm.Length || z.LoopStart >= z.LoopEndExclusive) throw new ArgumentException("Invalid half-open loop range.");
    }
    private static void Write32(byte[] bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), value);
    public static uint Crc32(ReadOnlySpan<byte> data)
    {
        uint crc = uint.MaxValue;
        foreach (byte b in data) { crc ^= b; for (int i = 0; i < 8; i++) crc = (crc >> 1) ^ ((crc & 1) != 0 ? 0xedb88320u : 0); }
        return ~crc;
    }
}
