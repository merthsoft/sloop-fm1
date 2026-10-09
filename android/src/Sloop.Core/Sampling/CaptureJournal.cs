using System.Globalization;
using System.Text;

namespace Sloop.Core.Sampling;

/// <summary>Recoverable PCM capture. The journal survives until the WAV is adopted by the workspace.</summary>
public sealed class CaptureJournal : IDisposable
{
    public string JournalPath { get; }
    public string WavePath => System.IO.Path.ChangeExtension(JournalPath, ".wav");
    private string RawPath => System.IO.Path.ChangeExtension(JournalPath, ".pcm");
    public int SampleRate { get; }
    public int Channels { get; }
    private FileStream? writer;
    private long bytesSinceFlush;

    private CaptureJournal(string path, int rate, int channels)
    { JournalPath = path; SampleRate = rate; Channels = channels; }

    public static CaptureJournal Create(string directory, int rate, int channels, string source)
    {
        ValidateFormat(rate, channels);
        Directory.CreateDirectory(directory);
        var journal = new CaptureJournal(System.IO.Path.Combine(directory, Guid.NewGuid().ToString("N") + ".take"), rate, channels);
        File.WriteAllLines(journal.JournalPath, ["SLOOP-TAKE-1", rate.ToString(CultureInfo.InvariantCulture),
            channels.ToString(CultureInfo.InvariantCulture), Convert.ToBase64String(Encoding.UTF8.GetBytes(source)),
            DateTimeOffset.UtcNow.ToString("O", CultureInfo.InvariantCulture)]);
        journal.writer = new FileStream(journal.RawPath, FileMode.CreateNew, FileAccess.Write, FileShare.Read, 65536);
        return journal;
    }
    private static void ValidateFormat(int rate, int channels)
    {
        if (rate is < 8000 or > 192000 || channels is < 1 or > 2) throw new FormatException("Invalid capture format.");
    }
    public static CaptureJournal OpenPending(string path)
    {
        var id = System.IO.Path.GetFileNameWithoutExtension(path);
        if (id.Length != 32 || !id.All(Uri.IsHexDigit) || System.IO.Path.GetExtension(path) != ".take")
            throw new FormatException("Invalid take identity.");
        var fields = File.ReadAllLines(path);
        if (fields.Length != 5 || fields[0] != "SLOOP-TAKE-1") throw new FormatException("Invalid take journal.");
        var rate = int.Parse(fields[1], CultureInfo.InvariantCulture);
        var channels = int.Parse(fields[2], CultureInfo.InvariantCulture);
        ValidateFormat(rate, channels);
        _ = Convert.FromBase64String(fields[3]);
        _ = DateTimeOffset.Parse(fields[4], CultureInfo.InvariantCulture);
        return new(path, rate, channels);
    }
    public void Append(ReadOnlySpan<byte> bytes)
    {
        if (writer is null) throw new InvalidOperationException("Take is closed.");
        if (bytes.Length % (Channels * 2) != 0) throw new ArgumentException("Capture chunks must contain complete frames.");
        writer.Write(bytes); bytesSinceFlush += bytes.Length;
        if (bytesSinceFlush >= SampleRate * Channels * 2) { writer.Flush(); bytesSinceFlush = 0; }
    }
    public string FinalizeWave()
    {
        Dispose();
        if (File.Exists(WavePath))
        {
            var existing = PcmWave.Open(WavePath);
            if (existing.SampleRate != SampleRate || existing.Channels != Channels) throw new FormatException("Recovered WAV format mismatch.");
            return WavePath;
        }
        using var input = File.OpenRead(RawPath);
        var bytes = input.Length / (Channels * 2) * (Channels * 2);
        if (bytes == 0) throw new IOException("No complete audio frames were captured.");
        if (bytes > uint.MaxValue - 36) throw new IOException("Capture exceeds WAV size limit.");
        var temp = WavePath + ".tmp";
        using (var output = File.Create(temp))
        using (var header = new BinaryWriter(output, Encoding.ASCII, leaveOpen: true))
        {
            header.Write(Encoding.ASCII.GetBytes("RIFF")); header.Write((uint)bytes + 36);
            header.Write(Encoding.ASCII.GetBytes("WAVEfmt ")); header.Write(16u); header.Write((ushort)1);
            header.Write((ushort)Channels); header.Write(SampleRate); header.Write(SampleRate * Channels * 2);
            header.Write((ushort)(Channels * 2)); header.Write((ushort)16);
            header.Write(Encoding.ASCII.GetBytes("data")); header.Write((uint)bytes);
            var buffer = new byte[65536]; long remaining = bytes;
            while (remaining > 0)
            {
                int read = input.Read(buffer, 0, (int)Math.Min(buffer.Length, remaining));
                if (read == 0) throw new EndOfStreamException("Capture PCM was truncated.");
                output.Write(buffer, 0, read); remaining -= read;
            }
            output.Flush(flushToDisk: true);
        }
        File.Move(temp, WavePath); return WavePath;
    }
    /// <summary>Call only after the workspace durably references the finalized WAV.</summary>
    public void Commit(string outcome)
    {
        _ = PcmWave.Open(WavePath);
        var metadata = WavePath + ".capture";
        File.WriteAllLines(metadata + ".tmp", File.ReadAllLines(JournalPath)
            .Append(Convert.ToBase64String(Encoding.UTF8.GetBytes(outcome))));
        File.Move(metadata + ".tmp", metadata, true);
        File.Delete(RawPath); File.Delete(JournalPath);
    }
    public void Dispose() { writer?.Dispose(); writer = null; }
}
