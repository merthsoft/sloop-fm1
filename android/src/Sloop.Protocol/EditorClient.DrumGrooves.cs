namespace Sloop.Protocol;

public sealed record DrumGroove(byte Id, string Name, byte Length, byte Division);
public enum DrumGrooveStatus : byte { Applied, Invalid, Busy, ConfirmationRequired }

public sealed partial class EditorClient
{
    public async Task<IReadOnlyList<DrumGroove>> ListDrumGroovesAsync(DeviceInfo info, CancellationToken token = default)
    {
        if (info.ProtocolVersion < 12) throw new NotSupportedException("Update SLOOP firmware to browse its groove bank.");
        var caps = await RequestAsync(74, [0], cancellationToken: token).ConfigureAwait(false);
        var a = caps.Arguments;
        if (a.Length != 5 || a[0] != 0 || a[1] != 0 || a[2] != 1 || a[3] is < 1 or > 24 || a[4] != 1)
            throw new FormatException("Unsupported drum groove bank capabilities.");
        var entries = DecodeDrumGrooves(await RequestAsync(74, [1], cancellationToken: token).ConfigureAwait(false));
        if (entries.Count != a[3]) throw new FormatException("Groove bank changed during discovery.");
        return entries;
    }

    public static IReadOnlyList<DrumGroove> DecodeDrumGrooves(EditorFrame frame)
    {
        if (frame.Command != 74) throw new FormatException("Expected drum groove bank.");
        var r = new PayloadReader(frame.Arguments);
        if (r.Byte() != 1 || r.Byte() != 0) throw new FormatException("Drum groove bank unavailable.");
        int count = r.Byte();
        if (count is < 1 or > 24) throw new FormatException("Invalid groove count.");
        var entries = new List<DrumGroove>(count);
        for (int i = 0; i < count; i++) {
            byte id = r.Byte(), length = r.Byte(), division = r.Byte();
            string name = r.String();
            if (id != i || length is < 1 or > 64 || division > 15 || string.IsNullOrWhiteSpace(name) ||
                name.Length > 20 || name.Any(c => c < 32 || c > 126)) throw new FormatException("Invalid groove entry.");
            entries.Add(new(id, name, length, division));
        }
        if (r.Remaining != 0) throw new FormatException("Trailing groove bank data.");
        return entries.AsReadOnly();
    }

    public async Task<DrumGrooveStatus> ApplyDrumGrooveAsync(byte id, bool replacementConfirmed, CancellationToken token = default)
    {
        if (id >= 24) throw new ArgumentOutOfRangeException(nameof(id));
        var frame = await RequestAsync(74, [2, id, (byte)(replacementConfirmed ? 1 : 0)],
            cancellationToken: token).ConfigureAwait(false);
        var a = frame.Arguments;
        if (a.Length != 3 || a[0] != 2 || a[2] != id || a[1] > 3)
            throw new FormatException("Invalid groove apply acknowledgement. Read hardware before trying again.");
        return (DrumGrooveStatus)a[1];
    }
}
