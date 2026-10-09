using System.Collections.Immutable;
using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using Sloop.Sequencing;

namespace Sloop.Workstation;

public enum CompositionPart { Bass, Chords, Melody, Drums }
public enum CompositionDensity { Sparse, Steady, Busy }
public enum CompositionProgression { Classic, Pop, MinorTurnaround }
public enum CompositionRhythm { Straight, Offbeat, Swing }
public sealed record CompositionIntent(int Seed, int Key, PerformanceScale Scale, int Bars,
    ImmutableArray<CompositionPart> Parts, CompositionDensity Density, int Tempo = 100,
    CompositionProgression Progression = CompositionProgression.Classic, CompositionRhythm Rhythm = CompositionRhythm.Straight);
public sealed record CompositionDraft(string Prompt, CompositionIntent Intent, AppPattern Pattern, string Generator = OfflineComposition.Version);

/// <summary>Bounded symbolic templates; no inference, downloads, audio or device operations.</summary>
public static class OfflineComposition
{
    public const string Version = "offline-composition/1";
    public const string Vocabulary = "key=C|C#|Db|D|D#|Eb|E|F|F#|Gb|G|G#|Ab|A|A#|Bb|B scale=major|minor|dorian|mixolydian|harmonic-minor|major-pentatonic|minor-pentatonic bars=1..16 seed=0..2147483647 parts=bass,chords,melody,drums density=sparse|steady|busy. All six fields required; optional tempo=40..240 progression=classic|pop|minor-turnaround rhythm=straight|offbeat|swing. Separated by spaces. No additional language supported.";
    static readonly Dictionary<string,int> Keys = new(StringComparer.OrdinalIgnoreCase) {
        ["C"]=0,["C#"]=1,["Db"]=1,["D"]=2,["D#"]=3,["Eb"]=3,["E"]=4,["F"]=5,["F#"]=6,["Gb"]=6,["G"]=7,["G#"]=8,["Ab"]=8,["A"]=9,["A#"]=10,["Bb"]=10,["B"]=11 };
    static readonly Dictionary<string,PerformanceScale> Scales = new(StringComparer.OrdinalIgnoreCase) {
        ["major"]=PerformanceScale.Major,["minor"]=PerformanceScale.NaturalMinor,["dorian"]=PerformanceScale.Dorian,["mixolydian"]=PerformanceScale.Mixolydian,["harmonic-minor"]=PerformanceScale.HarmonicMinor,["major-pentatonic"]=PerformanceScale.MajorPentatonic,["minor-pentatonic"]=PerformanceScale.MinorPentatonic };
    public static int Channel(CompositionPart part) => part switch {
        CompositionPart.Bass=>0, CompositionPart.Chords=>1, CompositionPart.Melody=>2, CompositionPart.Drums=>9,
        _=>throw new EditException("Unknown composition part.") };
    public static CompositionIntent Parse(string prompt)
    {
        if (string.IsNullOrWhiteSpace(prompt) || prompt.Length>512) throw new EditException("Composition prompt must contain 1..512 characters.");
        var fields=new Dictionary<string,string>(StringComparer.OrdinalIgnoreCase);
        foreach(var word in prompt.Split((char[]?)null,StringSplitOptions.RemoveEmptyEntries)) {
            var pair=word.Split('=');
            if(pair.Length!=2 || pair[1].Length==0 || !fields.TryAdd(pair[0],pair[1])) throw new EditException("Use each supported field exactly once. "+Vocabulary);
        }
        string[] names=["seed","key","scale","bars","parts","density"];
        if(names.Any(n=>!fields.ContainsKey(n)) || fields.Keys.Any(n=>!names.Contains(n.ToLowerInvariant()) && n.ToLowerInvariant() is not ("tempo" or "progression" or "rhythm"))) throw new EditException("Unsupported or missing composition fields. "+Vocabulary);
        if(!int.TryParse(fields["seed"],NumberStyles.None,CultureInfo.InvariantCulture,out var seed) || seed<0 ||
           !int.TryParse(fields["bars"],NumberStyles.None,CultureInfo.InvariantCulture,out var bars) || bars is <1 or >16 ||
           !Keys.TryGetValue(fields["key"],out var key) || !Scales.TryGetValue(fields["scale"],out var scale)) throw new EditException("Invalid seed, key, scale or bars. "+Vocabulary);
        var parts=fields["parts"].Split(',').Select(p=>p.ToLowerInvariant() switch {
            "bass"=>CompositionPart.Bass,"chords"=>CompositionPart.Chords,"melody"=>CompositionPart.Melody,"drums"=>CompositionPart.Drums,
            _=>throw new EditException("Unsupported part: "+p) }).Order().ToImmutableArray();
        if(parts.Distinct().Count()!=parts.Length) throw new EditException("Parts must be unique.");
        var density=fields["density"].ToLowerInvariant() switch { "sparse"=>CompositionDensity.Sparse,"steady"=>CompositionDensity.Steady,"busy"=>CompositionDensity.Busy,_=>throw new EditException("Unsupported density.") };
        int tempo=100;
        if(fields.TryGetValue("tempo",out var tempoText) && (!int.TryParse(tempoText,NumberStyles.None,CultureInfo.InvariantCulture,out tempo) || tempo is <40 or >240))throw new EditException("Tempo must be 40..240 BPM.");
        var progression=fields.GetValueOrDefault("progression","classic").ToLowerInvariant() switch {"classic"=>CompositionProgression.Classic,"pop"=>CompositionProgression.Pop,"minor-turnaround"=>CompositionProgression.MinorTurnaround,_=>throw new EditException("Unsupported progression.")};
        var rhythm=fields.GetValueOrDefault("rhythm","straight").ToLowerInvariant() switch {"straight"=>CompositionRhythm.Straight,"offbeat"=>CompositionRhythm.Offbeat,"swing"=>CompositionRhythm.Swing,_=>throw new EditException("Unsupported rhythm.")};
        return new(seed,key,scale,bars,parts,density,tempo,progression,rhythm);
    }
    static void Validate(CompositionIntent intent) {
        if(intent.Seed<0 || intent.Key is <0 or >11 || !Enum.IsDefined(intent.Scale) || intent.Bars is <1 or >16 ||
           !Enum.IsDefined(intent.Density) || intent.Tempo is <40 or >240 || !Enum.IsDefined(intent.Progression) || !Enum.IsDefined(intent.Rhythm) || intent.Parts.IsDefaultOrEmpty || intent.Parts.Distinct().Count()!=intent.Parts.Length || intent.Parts.Any(p=>!Enum.IsDefined(p)))
            throw new EditException("Invalid composition intent.");
    }
    static Guid Identity(string value)=>new(SHA256.HashData(Encoding.UTF8.GetBytes(Version+":"+value)).AsSpan(0,16));
    public static CompositionDraft Generate(string prompt,int ticksPerQuarter=960,CancellationToken cancellationToken=default)
        =>Generate(prompt,Parse(prompt),ticksPerQuarter,cancellationToken);
    static CompositionDraft Generate(string prompt,CompositionIntent intent,int ticksPerQuarter,CancellationToken token)
    {
        Validate(intent); token.ThrowIfCancellationRequested();
        if(ticksPerQuarter is <4 or >96000 || ticksPerQuarter%4!=0) throw new EditException("Composition requires a quarter-note resolution divisible by four (4..96000).");
        long step=ticksPerQuarter/4, length=step*16*intent.Bars;
        string signature=$"{intent.Seed}/{intent.Key}/{intent.Scale}/{intent.Bars}/{intent.Density}/{ticksPerQuarter}";
        if(intent.Progression!=CompositionProgression.Classic || intent.Rhythm!=CompositionRhythm.Straight)signature+=$"/{intent.Progression}/{intent.Rhythm}";
        var notes=ImmutableArray.CreateBuilder<AppNote>(); int[] progression=intent.Progression switch {CompositionProgression.Pop=>[0,4,5,3],CompositionProgression.MinorTurnaround=>[0,5,3,4],_=>[0,3,4,0]}; int[]? previous=null;
        foreach(var part in intent.Parts.Order()) {
            var partId=Identity("part/"+part); uint state=unchecked((uint)intent.Seed+1+(uint)part*2654435761u);
            int Next(int limit){state=unchecked(state*1664525u+1013904223u);return (int)(state%(uint)limit);}
            void Add(int bar,int position,int duration,int pitch,int velocity) {
                var id=Identity($"{signature}/{part}/{bar}/{position}/{pitch}");
                long onset=(bar*16+position)*step;
                if(intent.Rhythm==CompositionRhythm.Swing && position%2==1)onset+=step/3;
                if(intent.Rhythm==CompositionRhythm.Offbeat && part!=CompositionPart.Drums)onset+=step;
                long noteLength=Math.Min(duration*step,length-onset);
                if(noteLength>0)notes.Add(new(id,partId,new(onset),new(noteLength),pitch,velocity,Channel(part)));
            }
            for(int bar=0;bar<intent.Bars;bar++) {
                token.ThrowIfCancellationRequested(); int degree=progression[bar%4];
                if(part==CompositionPart.Chords) {
                    var chord=PerformanceHarmony.Chord(intent.Key,intent.Scale,4,degree,ChordShape.Diatonic,0,true,previous); previous=chord.Notes;
                    int spacing=intent.Density==CompositionDensity.Busy?8:16;
                    for(int pos=0;pos<16;pos+=spacing)foreach(int pitch in chord.Notes)Add(bar,pos,spacing-1,pitch,72+Next(12));
                } else if(part==CompositionPart.Drums) {
                    for(int pos=0;pos<16;pos++) {
                        if(pos%8==0)Add(bar,pos,1,36,100+Next(12));
                        if(pos is 4 or 12)Add(bar,pos,1,38,94+Next(12));
                        int spacing=intent.Density==CompositionDensity.Sparse?4:intent.Density==CompositionDensity.Steady?2:1;
                        if(pos%spacing==0)Add(bar,pos,1,42,60+Next(25));
                    }
                } else {
                    int spacing=intent.Density==CompositionDensity.Sparse?8:intent.Density==CompositionDensity.Steady?4:2;
                    for(int pos=0;pos<16;pos+=spacing) {
                        int pitch=part==CompositionPart.Bass?PerformanceHarmony.Chord(intent.Key,intent.Scale,3,degree,ChordShape.Diatonic,0,false).Bass:
                            PerformanceHarmony.ScaleNote(intent.Key,intent.Scale,5,Next(PerformanceHarmony.Scale(intent.Scale).Length));
                        Add(bar,pos,spacing-1,pitch,part==CompositionPart.Bass?90+Next(15):76+Next(20));
                    }
                }
            }
        }
        var pattern=new AppPattern(Identity("draft/"+signature),Identity("revision/"+signature),ticksPerQuarter,new(length),notes.ToImmutable());
        PatternValidation.Validate(pattern);return new(prompt,intent,pattern);
    }
    public static CompositionDraft EditNote(CompositionDraft draft,Guid id,AppNote? note)
    {
        var old=draft.Pattern.Notes.SingleOrDefault(n=>n.Id==id) ?? throw new EditException("Unknown draft note.");
        if(note is not null && (note.PartId!=old.PartId || note.Channel!=old.Channel)) throw new EditException("Draft note must retain its part and channel.");
        return draft with {Pattern=(AppPattern)AppEditor.PutNote(draft.Pattern,id,note).After};
    }
    public static CompositionDraft Regenerate(CompositionDraft draft,ImmutableHashSet<CompositionPart> keep,CancellationToken token=default)
    {
        if(keep.Any(p=>!draft.Intent.Parts.Contains(p)))throw new EditException("Cannot keep a part absent from the draft.");
        if(draft.Intent.Seed==int.MaxValue)throw new EditException("Seed is at its maximum; create a new draft with a lower seed.");
        var intent=draft.Intent with {Seed=draft.Intent.Seed+1};
        var prompt=string.Join(" ",draft.Prompt.Split((char[]?)null,StringSplitOptions.RemoveEmptyEntries)
            .Select(field=>field.StartsWith("seed=",StringComparison.OrdinalIgnoreCase)?"seed="+intent.Seed.ToString(CultureInfo.InvariantCulture):field));
        var next=Generate(prompt,intent,draft.Pattern.TicksPerQuarter,token);
        var channels=keep.Select(Channel).ToHashSet();
        var pattern=next.Pattern with {Notes=next.Pattern.Notes.Where(n=>!channels.Contains(n.Channel)).Concat(draft.Pattern.Notes.Where(n=>channels.Contains(n.Channel))).ToImmutableArray()};
        PatternValidation.Validate(pattern);return next with {Pattern=pattern};
    }
    public static void ValidateDraft(CompositionDraft draft)
    {
        if(draft is null || draft.Intent is null || draft.Pattern is null || draft.Pattern.Notes.IsDefault || draft.Pattern.Notes.Any(n=>n is null))throw new EditException("Invalid draft structure.");
        if(draft.Generator!=Version)throw new EditException("Unsupported composition generator.");
        Validate(draft.Intent);PatternValidation.Validate(draft.Pattern);
        var parsed=Parse(draft.Prompt);
        if(parsed with {Parts=draft.Intent.Parts} != draft.Intent || !parsed.Parts.SequenceEqual(draft.Intent.Parts))throw new EditException("Draft prompt differs from resolved intent.");
        if(draft.Pattern.TicksPerQuarter is <4 or >96000 || draft.Pattern.TicksPerQuarter%4!=0 || draft.Pattern.Length.Value!=(long)draft.Intent.Bars*4*draft.Pattern.TicksPerQuarter || draft.Pattern.Notes.Length>8192 || draft.Pattern.Notes.Any(n=>!draft.Intent.Parts.Select(Channel).Contains(n.Channel)))throw new EditException("Draft exceeds composition bounds.");
    }
    public static EditProposal ProposeApply(AppPattern source,CompositionDraft draft,EditLocks? locks=null)
    {
        PatternValidation.Validate(source);PatternValidation.Validate(draft.Pattern);Validate(draft.Intent);locks??=EditLocks.None;
        if(source.TicksPerQuarter!=draft.Pattern.TicksPerQuarter)throw new EditException("Draft timing resolution differs from the current pattern.");
        if(draft.Pattern.Length.Value!=(long)draft.Intent.Bars*4*draft.Pattern.TicksPerQuarter)throw new EditException("Draft length differs from its resolved intent.");
        var channels=draft.Intent.Parts.Select(Channel).ToHashSet();
        if(draft.Pattern.Notes.Any(n=>!channels.Contains(n.Channel)))throw new EditException("Draft contains an unrequested channel.");
        if(draft.Pattern.Notes.Any(n=>locks.Parts.Contains(n.PartId)||locks.Events.Contains(n.Id)))throw new EditException("Composition would add protected identities.");
        var removed=source.Notes.Where(n=>channels.Contains(n.Channel)).ToArray();
        if(removed.Any(n=>locks.Events.Contains(n.Id)||locks.Parts.Contains(n.PartId)))throw new EditException("Composition would replace protected notes.");
        var after=source with {Revision=Guid.NewGuid(),Length=draft.Pattern.Length,Notes=source.Notes.Where(n=>!channels.Contains(n.Channel)).Concat(draft.Pattern.Notes).ToImmutableArray()};
        // Retained material outside a shorter loop rejects rather than truncating it.
        PatternValidation.Validate(after);
        var proposal=EditProposal.Between(source,after,new($"Compose {draft.Intent.Bars} bars; replace MIDI channels {string.Join(",",channels.Order().Select(c=>c+1))}; key {PerformanceHarmony.Names[draft.Intent.Key]}, {draft.Intent.Scale}, {draft.Intent.Density}",Version,draft.Intent.Seed,draft.Prompt));
        if(proposal.Changes.IsEmpty && source.Length!=after.Length)throw new EditException("Length-only composition cannot be applied by note history. Retain at least one generated note.");
        return proposal;
    }
}
