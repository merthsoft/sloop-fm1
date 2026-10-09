using Sloop.SampleEncoding;

namespace Sloop.Workstation;

/// <summary>Source-order assignments, using the same root and midpoint rules as the slot builder.</summary>
public sealed record ChopAssignment(int ChopIndex, int RootNote, int LowNote, int HighNote)
{
    public string Description => $"Chop {ChopIndex + 1:00} → {SampleKitPlan.NoteName(RootNote)} ({RootNote}) · keys {LowNote}–{HighNote}";
}

public static class SampleKitPlan
{
    public static IReadOnlyList<ChopAssignment> Assignments(KitSettings settings, int chops, Sloop.Core.Sampling.SampleDocument? document=null, IReadOnlyList<ChopAudio>? edits=null)
    {
        Validate(settings, chops);
        var roots=Enumerable.Range(0,chops).Select(i=> { var slice=document?.Slices()[i]; return (slice.HasValue?ChopAudioSettings.Find(edits,slice.Value.Start,slice.Value.End)?.RootNote:null) ?? (settings.Mapping==SampleMapping.DrumLanes?SlotBuilder.DrumLaneNote(i):settings.RootNote+i); }).ToArray();
        if(roots.Distinct().Count()!=chops) throw new ArgumentException("Each chop needs a distinct MIDI root.");
        var sorted=Enumerable.Range(0,chops).OrderBy(i=>roots[i]).ToArray();
        return Enumerable.Range(0, chops).Select(i => {
            int root = roots[i]; int position=Array.IndexOf(sorted,i);
            int low = settings.Mapping == SampleMapping.Instrument ? (position == 0 ? 0 : (roots[sorted[position-1]]+root)/2+1) : root;
            int high = settings.Mapping == SampleMapping.Instrument ? (position == chops - 1 ? 127 : (root+roots[sorted[position+1]])/2) : root;
            return new ChopAssignment(i, root, low, high);
        }).ToArray();
    }

    public static void Validate(KitSettings settings, int chops)
    {
        ArgumentNullException.ThrowIfNull(settings);
        if (chops is < 1 or > 16) throw new ArgumentException("A kit needs 1–16 chops.");
        if (settings.Name is null || !Enum.IsDefined(settings.Mapping) || !Enum.IsDefined(settings.Mono)
            || settings.RootNote is < 0 or > 127 || !double.IsFinite(settings.Gain) || settings.Gain is < 0 or > 8)
            throw new ArgumentException("Use a valid mapping, root 0–127 and gain 0–8.");
        if (settings.Mapping != SampleMapping.DrumLanes && settings.RootNote + chops - 1 > 127)
            throw new ArgumentException($"{chops} chops need a first root of {128 - chops} or lower.");
    }

    public static string NoteName(int note)
    {
        if (note is < 0 or > 127) throw new ArgumentOutOfRangeException(nameof(note));
        string[] names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
        return $"{names[note % 12]}{note / 12 - 1}";
    }
}


