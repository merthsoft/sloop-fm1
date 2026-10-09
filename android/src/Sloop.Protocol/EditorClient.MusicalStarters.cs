namespace Sloop.Protocol;

public sealed record MusicalStarter(byte Id, string Name);
public sealed record MusicalStarterBank(IReadOnlyList<MusicalStarter> Entries, IReadOnlyList<MusicalStarter> Scales, int LeaseMilliseconds);
public enum MusicalStarterStatus : byte { Ok, Invalid, Busy, ConfirmationRequired, Stale }
public sealed record MusicalStarterOptions(byte Track, byte Id, byte Key = 0, byte Scale = 1,
    int Octave = 0, byte Mode = 0, bool VoiceLeading = false, int Rotate = 0, int Offset = 0,
    byte Syncopation = 0, int Feel = 0)
{
    public byte[] Encode(byte operation, bool confirmed = false)
    {
        if (Track > 2 || Id >= 24 || Key > 11 || Scale >= 32 || Octave is < -3 or > 3 || Mode > 2 ||
            Rotate is < -16 or > 16 || Offset is < -8 or > 8 || Syncopation > 2 || Feel is < -32 or > 31)
            throw new ArgumentOutOfRangeException(nameof(MusicalStarterOptions));
        return [operation, Track, Id, Key, Scale, (byte)(Octave + 3), Mode, (byte)(VoiceLeading ? 1 : 0),
            (byte)(Rotate + 16), (byte)(Offset + 8), Syncopation, (byte)(Feel + 32), (byte)(confirmed ? 1 : 0)];
    }
}
public sealed partial class EditorClient
{
    public async Task<MusicalStarterBank> ListMusicalStartersAsync(DeviceInfo info, CancellationToken token = default)
    {
        if (info.ProtocolVersion < 14) throw new NotSupportedException("Update SLOOP firmware for its musical starter library.");
        var a = (await RequestAsync(76, [0], cancellationToken: token).ConfigureAwait(false)).Arguments;
        if (a.Length != 8 || a[0] != 0 || a[1] != 0 || a[2] != 1 || a[3] != 24 || a[4] != 7 || a[5] is < 1 or > 32 ||
            (a[6] | a[7] << 7) != 1500) throw new NotSupportedException("Firmware does not advertise the supported musical starter capability.");
        var entries = DecodeMusicalStarterNames(await RequestAsync(76, [1], cancellationToken: token).ConfigureAwait(false), 1);
        var scales = DecodeMusicalStarterNames(await RequestAsync(76, [5], cancellationToken: token).ConfigureAwait(false), 5);
        if (entries.Count != a[3] || scales.Count != a[5]) throw new FormatException("Starter capabilities changed during discovery.");
        return new(entries, scales, 1500);
    }
    public static IReadOnlyList<MusicalStarter> DecodeMusicalStarterNames(EditorFrame frame, byte operation)
    {
        if (frame.Command != 76 || operation is not (1 or 5)) throw new FormatException("Expected musical starter names.");
        var r = new PayloadReader(frame.Arguments);
        if (r.Byte() != operation || r.Byte() != 0) throw new FormatException("Musical starter names unavailable.");
        int count = r.Byte();
        if (count < 1 || count > (operation == 1 ? 24 : 32)) throw new FormatException("Invalid name count.");
        var names = new List<MusicalStarter>();
        for (int i = 0; i < count; i++) {
            byte id = r.Byte(); string name = r.String();
            if (id != i || string.IsNullOrWhiteSpace(name) || name.Length > 20 || name.Any(c => c < 32 || c > 126))
                throw new FormatException("Invalid starter name.");
            names.Add(new(id, name));
        }
        if (r.Remaining != 0) throw new FormatException("Trailing starter data.");
        return names.AsReadOnly();
    }
    static ushort ValidateStarterLease(ushort lease)
    {
        if (lease is 0 or > 16383) throw new ArgumentOutOfRangeException(nameof(lease));
        return lease;
    }
    async Task<MusicalStarterStatus> StarterRequestAsync(byte[] request, CancellationToken token)
    {
        var a = (await RequestAsync(76, request, cancellationToken: token).ConfigureAwait(false)).Arguments;
        if (a.Length != 2 || a[0] != request[0] || a[1] > 4) throw new FormatException("Invalid starter acknowledgement. Read hardware before retrying.");
        return (MusicalStarterStatus)a[1];
    }
    public Task<MusicalStarterStatus> ApplyMusicalStarterAsync(MusicalStarterOptions options, bool confirmed, CancellationToken token = default) =>
        StarterRequestAsync(options.Encode(2, confirmed), token);
    public Task<MusicalStarterStatus> PreviewMusicalStarterAsync(MusicalStarterOptions options, ushort lease, CancellationToken token = default)
    {
        ValidateStarterLease(lease);
        return StarterRequestAsync([..options.Encode(3), (byte)(lease & 127), (byte)(lease >> 7)], token);
    }
    public Task<MusicalStarterStatus> MusicalStarterLeaseAsync(ushort lease, bool renew, CancellationToken token = default)
    {
        ValidateStarterLease(lease);
        return StarterRequestAsync([4, (byte)(lease & 127), (byte)(lease >> 7), (byte)(renew ? 1 : 0)], token);
    }
}
