namespace Sloop.Protocol;

public enum HardwareGestureKind : byte { Fill, Punch, NextBarFill }
public enum PerformanceStatus : byte { Ok, Malformed, Full, Missing }
public sealed record PerformanceCapabilities(byte Schema, byte Flags, byte Owners, int LeaseMilliseconds, byte Effects)
{
    public bool Supported => Schema == 1 && Flags == 7 && Owners is > 0 and <= 8 && LeaseMilliseconds is >= 250 and <= 2000 && Effects == 16;
}
public static class PerformanceWire
{
    public const byte Command = 73;
    public static byte[] Token(uint token)
    {
        if (token is 0 or > 0xfffffff) throw new ArgumentOutOfRangeException(nameof(token));
        return [(byte)(token & 127), (byte)((token >> 7) & 127), (byte)((token >> 14) & 127), (byte)((token >> 21) & 127)];
    }
    public static byte[] Press(uint token, HardwareGestureKind kind, byte value = 0)
    {
        if (kind > HardwareGestureKind.NextBarFill || (kind == HardwareGestureKind.Punch ? value >= 16 : value != 0)) throw new ArgumentOutOfRangeException(nameof(value));
        return [1, .. Token(token), (byte)kind, value];
    }
    public static PerformanceCapabilities ParseCapabilities(EditorFrame frame)
    {
        var a = frame.Arguments;
        if (frame.Command != Command || a.Length != 8 || a.Any(b => b > 127) || a[0] != 0 || a[1] != 0) throw new FormatException("Malformed performance discovery.");
        return new(a[2], a[3], a[4], a[5] | a[6] << 7, a[7]);
    }
    public static PerformanceStatus ParseReply(EditorFrame frame, byte op, uint token)
    {
        var a = frame.Arguments;
        if (frame.Command != Command || a.Length != 6 || a.Any(b => b > 127) || a[0] != op || a[1] > 3 ||
            (uint)(a[2] | a[3] << 7 | a[4] << 14 | a[5] << 21) != token) throw new FormatException("Malformed performance reply.");
        return (PerformanceStatus)a[1];
    }
}
