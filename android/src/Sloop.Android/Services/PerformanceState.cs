using Sloop.Protocol;

namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    public int? HardwareOctave { get; private set; }
    public event Action? HardwareOctaveChanged;

    private async Task FollowHardwareOctaveAsync(EditorClient editor, CancellationTokenSource current)
    {
        var token = current.Token;
        try
        {
            while (true)
            {
                await Task.Delay(100, token);
                if (epoch != current) return;
                if (Snapshot.Busy) continue;
                int offset = await editor.GetHardwareOctaveAsync(token);
                if (epoch != current) return;
                if (HardwareOctave == offset) continue;
                HardwareOctave = offset;
                HardwareOctaveChanged?.Invoke();
            }
        }
        catch (OperationCanceledException) when (epoch != current || token.IsCancellationRequested) { }
        catch (Exception error)
        {
            if (epoch == current) Fail("Hardware octave synchronization failed: " + error.Message);
        }
    }
}
