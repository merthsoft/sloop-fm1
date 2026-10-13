using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Workstation;

public sealed record MidiHardwareImport(HardwarePattern Pattern, AppPattern Preview, string Report);
public static class MidiFileHardware
{
    // Standard General MIDI drum notes 35–81 mapped to the FM1's sixteen lanes.
    static readonly int[] lanes = [1,0,7,2,3,8,9,4,9,6,9,5,10,10,11,10,12,11,12,13,11,15,11,13,12,14,14,14,14,14,14,14,15,15,13,13,15,15,13,13,7,7,7,14,14,15,15];
    static readonly int[] gmNotes = [36,35,38,39,42,46,44,37,40,43,48,49,51,70,63,56];
    static readonly double[] perBar = [4,8,16,32,12,24,2,1,.5];
    public static MidiHardwareImport Convert(MidiFileDocument file, MidiFilePart part, HardwarePattern before, int destination)
    {
        PatternValidation.Validate(before);
        if (destination is < 0 or > 3) throw new ArgumentOutOfRangeException(nameof(destination));
        var track = before.Tracks[destination];
        if (!track.Parameters.TryGetValue(30, out int division) || division < 0 || division >= perBar.Length) throw new InvalidDataException("Read a track with a supported DIV before importing MIDI.");
        double ticks = file.Ppq * 4 / perBar[division];
        double previewTicks = 960 * 4 / perBar[division];
        int dropped = 0, shifted = 0, duplicates = 0, excess = 0, overlaps = 0, durations = 0;
        var grouped = new SortedDictionary<int,List<(MidiFileNote Note,int End)>>();
        foreach (var note in part.Notes) {
            int start = checked((int)Math.Round(note.Start / ticks, MidpointRounding.AwayFromZero));
            if (start >= track.Length) { dropped++; continue; }
            if (Math.Abs(start * ticks - note.Start) > .25 * ticks) shifted++;
            int end = Math.Max(start + 1, checked((int)Math.Round((note.Start + note.Duration) / ticks, MidpointRounding.AwayFromZero)));
            if (end > track.Length) durations++;
            if (!grouped.TryGetValue(start, out var list)) grouped[start] = list = [];
            list.Add((note, Math.Min(track.Length, end)));
        }
        if (grouped.Count == 0) throw new InvalidDataException("No notes fit this track's active length. Increase its length or choose another MIDI part.");
        // Keep dormant steps and sound/timing/lock settings. Replace only active step content.
        var steps = track.Steps.ToBuilder();
        for (int i = 0; i < track.Length; i++) steps[i] = HardwareEditor.Blank(track.IsDrum);
        var audition = ImmutableArray.CreateBuilder<AppNote>();
        int[] starts = grouped.Keys.ToArray();
        for (int index = 0; index < starts.Length; index++) {
            int start = starts[index]; var list = grouped[start];
            if (track.IsDrum) {
                var step = (DrumStep)steps[start]; var hits = step.Lanes.ToBuilder();
                foreach (var lane in list.GroupBy(n => n.Note.Pitch < 35 ? 0 : n.Note.Pitch > 81 ? 13 : lanes[n.Note.Pitch - 35])) {
                    duplicates += lane.Count() - 1; int velocity = lane.Max(n => n.Note.Velocity);
                    var level = velocity < 56 ? HitLevel.Ghost : velocity < 88 ? HitLevel.Soft : velocity < 116 ? HitLevel.Normal : HitLevel.Hard;
                    hits[lane.Key] = hits[lane.Key] with { On = true, Level = level };
                    audition.Add(new(Guid.NewGuid(), before.Id, new((long)Math.Round(start * previewTicks)), new(Math.Max(1,(long)Math.Round(previewTicks / 4))), gmNotes[lane.Key], level switch {HitLevel.Ghost=>42,HitLevel.Soft=>72,HitLevel.Hard=>127,_=>100},9));
                }
                steps[start] = step with { Lanes = hits.ToImmutable() };
            } else {
                var unique = list.GroupBy(n => n.Note.Pitch).Select(g => g.OrderByDescending(n => n.Note.Velocity).First()).ToArray();
                duplicates += list.Count - unique.Length; excess += Math.Max(0,unique.Length-4);
                var chord = unique.OrderByDescending(n => n.Note.Velocity).Take(4).OrderBy(n => n.Note.Pitch).ToArray();
                if (chord.Select(n => n.End).Distinct().Count() > 1) durations++;
                int end = chord.Max(n => n.End), next = index+1 < starts.Length ? starts[index+1] : track.Length;
                if (end > next) { overlaps++; end = next; }
                var step = (SynthStep)steps[start]; var slots = step.Slots.ToBuilder();
                int velocity = chord.Max(n => n.Note.Velocity);
                for (int i = 0; i < chord.Length; i++) {
                    slots[i] = slots[i] with { Pitch = chord[i].Note.Pitch };
                    audition.Add(new(Guid.NewGuid(),before.Id,new((long)Math.Round(start*previewTicks)),new(Math.Max(1,(long)Math.Round((end-start)*previewTicks))),chord[i].Note.Pitch,velocity,destination));
                }
                steps[start] = step with { Count = chord.Length, Time = StepTime.Note, Velocity = velocity, Slots = slots.ToImmutable() };
                for (int i = start+1; i < end; i++) steps[i] = ((SynthStep)steps[i]) with {Time=StepTime.Tie};
            }
        }
        var after = before with { Revision=Guid.NewGuid(), Tracks=before.Tracks.SetItem(destination,track with {Steps=steps.ToImmutable()}) };
        var preview = new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(Math.Max(1,(long)Math.Ceiling(track.Length*previewTicks))),audition.ToImmutable());
        PatternValidation.Validate(after); PatternValidation.Validate(preview);
        string report = $"Replaces track {destination+1}'s {track.Length} active steps at its current DIV. Other tracks, dormant steps, sound, length and parameter locks stay intact. Active micro timing/fill/ratchets reset.\n"+
            $"{dropped} notes beyond active length · {shifted} onsets moved >¼ step · {duplicates} duplicates merged · {excess} notes beyond four per chord · {overlaps} chords shortened by next onset · {durations} duration groups extended/clipped.\n"+
            (track.IsDrum ? "GM drum pitches map to FM1 lanes; velocities become four hit levels. Preview uses the mapped lanes." : "Each chord shares its longest duration and strongest velocity; ties end at the next onset. Preview reflects this conversion.");
        return new(after,preview,report);
    }
}
