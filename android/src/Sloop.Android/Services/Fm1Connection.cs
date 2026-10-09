using Android.Content;
using Android.Media.Midi;
using Android.OS;
using Sloop.Protocol;
using Sloop.Simulator;

namespace Sloop.Android.Services;

public sealed record ConnectionSnapshot(string Status, bool Busy, DeviceInfo? Device,
    ParameterDescriptor? LevelDescriptor, IReadOnlyList<int> Levels, bool IsSimulated = false, bool IsGenericMidi = false);
public sealed record MidiCandidate(MidiDeviceInfo Info, string Name);

/// <summary>Process-scoped link survives Activity rotation; all control calls use the main thread.</summary>
public sealed partial class Fm1Connection
{
    private static Fm1Connection? instance;
    public static Fm1Connection Get(Context context) => instance ??= new(context.ApplicationContext!);
    private readonly MidiManager? manager;
    private readonly Handler handler = new(Looper.MainLooper!);
    private readonly DeviceEvents deviceEvents;
    private CancellationTokenSource? epoch;
    private IDisposable? transport;
    private EditorClient? client;
    private int? deviceId;
    public ConnectionSnapshot Snapshot { get; private set; } = new("Offline · FM1 not connected", false, null, null, []);
    public event Action? Changed;

    private Fm1Connection(Context context)
    {
        manager = context.GetSystemService(Context.MidiService) as MidiManager;
        deviceEvents = new DeviceEvents(this);
        if (manager is not null)
        {
            if (OperatingSystem.IsAndroidVersionAtLeast(33))
                manager.RegisterDeviceCallback((int)MidiTransport.MidiByteStream, context.MainExecutor!, deviceEvents);
            else
                manager.RegisterDeviceCallback(deviceEvents, handler);
        }
        if (manager is null) Snapshot = Snapshot with { Status = "Android MIDI service is unavailable." };
    }

    public IReadOnlyList<MidiCandidate> Candidates(bool generic = false)
    {
        if (manager is null) return [];
        IEnumerable<MidiDeviceInfo> devices = OperatingSystem.IsAndroidVersionAtLeast(33)
            ? manager.GetDevicesForTransport((int)MidiTransport.MidiByteStream) ?? []
            : manager.GetDevices() ?? [];
        return devices.Where(d => d.InputPortCount > 0 && (generic || d.Type == MidiDeviceType.Usb && d.OutputPortCount > 0))
            .Select(d => new MidiCandidate(d, d.Properties?.GetString(MidiDeviceInfo.PropertyName) ?? "USB MIDI device"))
            .ToArray();
    }

    public async Task ConnectAsync(MidiCandidate candidate, bool generic = false)
    {
        if (manager is null || Snapshot.Busy) return;
        Disconnect();
        var current = epoch = new CancellationTokenSource();
        deviceId = candidate.Info.Id;
        Publish(new($"Opening {candidate.Name}…", true, null, null, []));
        try
        {
            var opened = await AndroidMidiTransport.OpenAsync(manager, candidate.Info, handler, current.Token, receive: !generic);
            if (epoch != current) { opened.Dispose(); return; }
            transport = opened;
            if (generic)
            {
                Publish(new($"MIDI connected · {candidate.Name} · generic mode",false,null,null,[],IsGenericMidi:true));
                return;
            }
            client = new EditorClient(opened);
            Publish(Snapshot with { Status = "Reading SLOOP device information…" });
            await HandshakeAsync(client, current, simulated: false);
        }
        catch (Exception error)
        {
            if (epoch == current) Fail(error.Message);
        }
    }

    public async Task ConnectSimulatedAsync()
    {
        if (Snapshot.Busy) return;
        Disconnect();
        var current = epoch = new CancellationTokenSource();
        Publish(new("Simulated FM1 · reading firmware profile…", true, null, null, [], true));
        try
        {
            var simulated = new SimulatedFm1();
            transport = simulated;
            client = new EditorClient(simulated);
            await HandshakeAsync(client, current, simulated: true);
        }
        catch (Exception error) { if (epoch == current) Fail(error.Message); }
    }

    private async Task HandshakeAsync(EditorClient editor, CancellationTokenSource current, bool simulated)
    {
        var info = await editor.GetInfoAsync(current.Token);
        // FL is shared with Felucca. Display the reported identity, not a guessed firmware name.
        if (epoch != current) return;
        ParameterDescriptor? descriptor = null;
        var hardwareOctave = !simulated && info.ProtocolVersion >= 10 ? (int?)await editor.GetHardwareOctaveAsync(current.Token) : null;
        if (epoch != current) return;
        HardwareOctave = hardwareOctave;
        int[] levels = [];
        if (info.ProtocolVersion >= 4 && info.TrackCount >= 3)
        {
            // P_LEVEL is zero in the existing protocol; verify before exposing control.
            var level = await editor.GetDescriptorAsync(0, 0, current.Token);
            if (level.Label == "LVL" && level.Minimum == 0 && level.Maximum == 127)
            {
                descriptor = level;
                levels = new int[3];
                for (byte track = 0; track < 3; track++)
                    levels[track] = await editor.TrackParameterAsync(track, level.Id, token: current.Token);
            }
        }
        if (epoch == current && !simulated)
            await NegotiateUsbPlaybackAsync(editor, info, current);
        if (epoch == current)
            Publish(new($"{(simulated ? "SIMULATED FM1" : "MIDI connected")} · {info.Firmware} · audio not configured",
                false, info, descriptor, Array.AsReadOnly(levels), simulated));
        if (epoch == current && !simulated)
            await InitializeHardwarePerformanceAsync(editor, current.Token);
        if (epoch == current && HardwareOctave is not null) _ = FollowHardwareOctaveAsync(editor, current);
    }

    public async Task SetLevelAsync(int track, int value)
    {
        if (Snapshot.Busy || client is null || epoch is null || Snapshot.LevelDescriptor is not { } descriptor ||
            track < 0 || track >= Snapshot.Levels.Count) return;
        if (value < descriptor.Minimum || value > descriptor.Maximum) throw new ArgumentOutOfRangeException(nameof(value));
        await RunLevelAsync(track, value);
    }

    public async Task RefreshLevelAsync(int track)
    {
        if (Snapshot.Busy || client is null || Snapshot.LevelDescriptor is null ||
            track < 0 || track >= Snapshot.Levels.Count) return;
        await RunLevelAsync(track, null);
    }

    private async Task RunLevelAsync(int track, int? value)
    {
        var current = epoch!;
        var previousStatus = Snapshot.Status;
        Publish(Snapshot with { Busy = true, Status = value is null ? "Reading track level…" : "Applying track level…" });
        try
        {
            var applied = await client!.TrackParameterAsync((byte)track, Snapshot.LevelDescriptor!.Id, value, current.Token);
            if (epoch != current) return;
            var levels = Snapshot.Levels.ToArray(); levels[track] = applied;
            Publish(Snapshot with { Busy = false, Status = previousStatus, Levels = Array.AsReadOnly(levels) });
        }
        catch (Exception error) { if (epoch == current) Fail($"{error.Message} Reconnect and read back the device state."); }
    }

    public void Disconnect()
    {
        _ = CloseHardwarePerformanceAsync();
        ResetUsbPlayback();
        ReleasePerformance();
        StopPlaying();
        soundFingerprints.Clear();
        HardwareOctave = null;
        epoch?.Cancel(); epoch?.Dispose(); epoch = null;
        deviceId = null;
        client?.Dispose(); client = null;
        transport?.Dispose(); transport = null;
        Publish(new("Offline · FM1 not connected", false, null, null, []));
    }

    private void Fail(string reason)
    {
        Disconnect();
        Publish(Snapshot with { Status = reason });
    }
    private void Publish(ConnectionSnapshot snapshot) { Snapshot = snapshot; Changed?.Invoke(); }

    private sealed class DeviceEvents(Fm1Connection owner) : MidiManager.DeviceCallback
    {
        public override void OnDeviceAdded(MidiDeviceInfo? device) => owner.Changed?.Invoke();
        public override void OnDeviceRemoved(MidiDeviceInfo? device)
        {
            if (device?.Id == owner.deviceId) owner.Fail("FM1 disconnected. Reconnect to refresh its state.");
            else owner.Changed?.Invoke();
        }
    }
}
