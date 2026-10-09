using System.Text;

namespace Sloop.Protocol;

public sealed record EditorFrame(byte Command, byte[] Arguments);

public static class EditorCodec
{
    public static byte[] Encode(byte command, ReadOnlySpan<byte> arguments)
    {
        if (command > 127) throw new ArgumentOutOfRangeException(nameof(command));
        foreach (var value in arguments)
            if (value > 127) throw new ArgumentException("SysEx arguments must be 7-bit.", nameof(arguments));
        var frame = new byte[arguments.Length + 6];
        frame[0] = 0xF0; frame[1] = 0x7D; frame[2] = 0x46; frame[3] = 0x4C;
        frame[4] = command;
        arguments.CopyTo(frame.AsSpan(5));
        frame[^1] = 0xF7;
        return frame;
    }

    public static byte[] EncodeValue(int value)
    {
        if (value is < -8192 or > 8191) throw new ArgumentOutOfRangeException(nameof(value));
        var encoded = value + 8192;
        return [(byte)(encoded & 127), (byte)(encoded >> 7)];
    }

    public static int DecodeValue(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length != 2 || bytes[0] > 127 || bytes[1] > 127)
            throw new FormatException("Invalid signed 14-bit value.");
        return (bytes[0] | bytes[1] << 7) - 8192;
    }

    public static byte[] Pack7(ReadOnlySpan<byte> source)
    {
        var output = new byte[source.Length + (source.Length + 6) / 7];
        var destination = 0;
        for (var start = 0; start < source.Length; start += 7)
        {
            var mask = destination++;
            for (var i = 0; i < Math.Min(7, source.Length - start); i++)
            {
                output[mask] |= (byte)((source[start + i] >> 7) << i);
                output[destination++] = (byte)(source[start + i] & 127);
            }
        }
        return output;
    }

    public static byte[] Unpack7(ReadOnlySpan<byte> source)
    {
        var output = new List<byte>(source.Length);
        var index = 0;
        while (index < source.Length)
        {
            var mask = source[index++];
            if (mask > 127 || index == source.Length) throw new FormatException("Invalid pack7 group.");
            var count = Math.Min(7, source.Length - index);
            if ((mask >> count) != 0) throw new FormatException("Unused pack7 mask bits are set.");
            for (var bit = 0; bit < count; bit++)
            {
                var value = source[index++];
                if (value > 127) throw new FormatException("Invalid pack7 data byte.");
                output.Add((byte)(value | ((mask >> bit) & 1) << 7));
            }
        }
        return output.ToArray();
    }
}

/// <summary>Bounded stream parser. Call Feed from one receive path at a time.</summary>
public sealed class EditorStreamParser(int maximumPayload = 4096)
{
    private readonly byte[] buffer = new byte[maximumPayload >= 4 ? maximumPayload :
        throw new ArgumentOutOfRangeException(nameof(maximumPayload))];
    private int length;
    private bool collecting;
    public event Action<EditorFrame>? FrameReceived;

    public void Feed(ReadOnlySpan<byte> bytes)
    {
        foreach (var value in bytes)
        {
            if (value >= 0xF8) continue; // MIDI realtime may interrupt SysEx.
            if (value == 0xF0) { collecting = true; length = 0; continue; }
            if (!collecting) continue;
            if (value == 0xF7)
            {
                collecting = false;
                if (length >= 4 && buffer[0] == 0x7D && buffer[1] == 0x46 && buffer[2] == 0x4C)
                    FrameReceived?.Invoke(new EditorFrame(buffer[3], buffer.AsSpan(4, length - 4).ToArray()));
                continue;
            }
            if (value > 127 || length == buffer.Length) { collecting = false; length = 0; continue; }
            buffer[length++] = value;
        }
    }
}

internal sealed class PayloadReader(byte[] bytes)
{
    private int position;
    public int Remaining => bytes.Length - position;
    public byte Byte() => position < bytes.Length ? bytes[position++] : throw new FormatException("Truncated reply.");
    public int Value() => EditorCodec.DecodeValue([Byte(), Byte()]);
    public string String()
    {
        var start = position;
        while (Byte() != 0) { }
        return Encoding.ASCII.GetString(bytes, start, position - start - 1);
    }
}
