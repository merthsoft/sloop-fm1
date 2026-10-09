using System.Text;

namespace Sloop.Core.Sampling;

/// <summary>File-backed PCM16 WAV; all editing coordinates are source frames.</summary>
public sealed record PcmWave(string Path, int SampleRate, int Channels, long DataOffset, long Frames)
{
    public static PcmWave Open(string path)
    {
        using var stream = File.OpenRead(path);
        using var reader = new BinaryReader(stream, Encoding.ASCII);
        string Tag() => Encoding.ASCII.GetString(reader.ReadBytes(4));
        if (stream.Length < 12 || Tag() != "RIFF") throw new FormatException("Choose a PCM16 WAV file.");
        long end = 8L + reader.ReadUInt32();
        if (Tag() != "WAVE" || end > stream.Length || end < 12) throw new FormatException("Truncated WAV file.");
        int rate = 0, channels = 0, alignment = 0;
        long offset = -1, length = 0;
        while (stream.Position + 8 <= end)
        {
            var tag = Tag();
            var size = reader.ReadUInt32();
            long next = stream.Position + size;
            if (next > end) throw new FormatException("Truncated WAV chunk.");
            if (tag == "fmt ")
            {
                if (size < 16 || reader.ReadUInt16() != 1) throw new FormatException("Only uncompressed PCM16 WAV is supported.");
                channels = reader.ReadUInt16();
                rate = reader.ReadInt32();
                var bytesPerSecond = reader.ReadInt32();
                alignment = reader.ReadUInt16();
                var bits = reader.ReadUInt16();
                if (channels is < 1 or > 2 || rate is < 8000 or > 192000 || bits != 16 ||
                    alignment != channels * 2 || bytesPerSecond != rate * alignment)
                    throw new FormatException("Use mono or stereo PCM16 WAV, 8–192 kHz.");
            }
            else if (tag == "data" && offset < 0) { offset = stream.Position; length = size; }
            stream.Position = next + (size & 1);
        }
        if (offset < 0 || alignment == 0 || length == 0 || length % alignment != 0)
            throw new FormatException("WAV has no complete PCM frames.");
        return new(path, rate, channels, offset, length / alignment);
    }

    public float[] Peaks(int count = 512)
    {
        if (count < 1 || count > 4096) throw new ArgumentOutOfRangeException(nameof(count));
        var peaks = new float[count];
        using var stream = File.OpenRead(Path);
        stream.Position = DataOffset;
        using var reader = new BinaryReader(stream);
        for (long frame = 0; frame < Frames; frame++)
        {
            var bucket = (int)(frame * count / Frames);
            for (var channel = 0; channel < Channels; channel++)
                peaks[bucket] = Math.Max(peaks[bucket], Math.Abs((int)reader.ReadInt16()) / 32768f);
        }
        return peaks;
    }

    public void Export(string destination, long start, long end)
    {
        if (start < 0 || end <= start || end > Frames) throw new ArgumentOutOfRangeException(nameof(start));
        if (System.IO.Path.GetFullPath(destination) == System.IO.Path.GetFullPath(Path))
            throw new ArgumentException("The original cannot be overwritten.");
        var bytes = checked((uint)((end - start) * Channels * 2));
        if (bytes > uint.MaxValue - 36) throw new ArgumentException("Slice exceeds WAV capacity.");
        using var source = File.OpenRead(Path);
        source.Position = DataOffset + start * Channels * 2;
        using var output = File.Create(destination);
        using var writer = new BinaryWriter(output, Encoding.ASCII, leaveOpen: true);
        writer.Write(Encoding.ASCII.GetBytes("RIFF")); writer.Write(bytes + 36);
        writer.Write(Encoding.ASCII.GetBytes("WAVEfmt ")); writer.Write(16u); writer.Write((ushort)1);
        writer.Write((ushort)Channels); writer.Write(SampleRate); writer.Write(SampleRate * Channels * 2);
        writer.Write((ushort)(Channels * 2)); writer.Write((ushort)16);
        writer.Write(Encoding.ASCII.GetBytes("data")); writer.Write(bytes);
        var buffer = new byte[65536];
        long remaining = bytes;
        while (remaining > 0)
        {
            var read = source.Read(buffer, 0, (int)Math.Min(remaining, buffer.Length));
            if (read == 0) throw new EndOfStreamException("Source was truncated.");
            output.Write(buffer, 0, read); remaining -= read;
        }
    }
}
