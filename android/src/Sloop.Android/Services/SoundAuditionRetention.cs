namespace Sloop.Android.Services;

/// <summary>Process lifetime recovery records, independent of Activity and local workspace lifetime.</summary>
public sealed class SoundAuditionRetention
{
    public static SoundAuditionRetention Current { get; } = new();
    readonly SoundAudition?[] auditions = new SoundAudition?[3];
    readonly bool[] replaced = new bool[3];
    public SoundAudition? this[int track] => auditions[track];
    public bool WorkspaceReplaced(int track) => replaced[track] && auditions[track]?.Active == true;
    public void Capture(int track, SoundAudition audition)
    {
        if(auditions[track]?.Active==true)throw new InvalidOperationException("Finish or abandon the retained audition first.");
        auditions[track]=audition;replaced[track]=false;
    }
    public void NotifyWorkspaceReplaced()
    {
        for(int track=0;track<3;track++)if(auditions[track]?.Active==true)replaced[track]=true;
    }
    public void NotifyDisconnected()
    {
        foreach(var audition in auditions)audition?.NotifyDisconnected();
    }
    public void Abandon(int track)
    {
        if(auditions[track]?.Busy==true)throw new InvalidOperationException("Wait for the current audition operation to finish.");
        auditions[track]=null;replaced[track]=false;
    }
}
