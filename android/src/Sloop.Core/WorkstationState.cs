namespace Sloop.Core;

public enum Workspace { Perform, Sequence, Sound, Sample, Library }

/// <summary>Local navigation only; does not represent connected hardware state.</summary>
public sealed class WorkstationState
{
    public Workspace Workspace { get; set; } = Workspace.Perform;
    public int SelectedTrack { get; private set; }
    public static IReadOnlyList<string> TrackNames { get; } =
        Array.AsReadOnly(new[] { "Synth 1", "Synth 2", "Synth 3", "Drums" });

    public void SelectTrack(int index)
    {
        if (index < 0 || index >= TrackNames.Count)
            throw new ArgumentOutOfRangeException(nameof(index));
        SelectedTrack = index;
    }
}
