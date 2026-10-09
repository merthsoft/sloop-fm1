using System.Collections.Immutable;
using System.Text.RegularExpressions;

namespace Sloop.SoundDesign;

public sealed record PhraseResult(SoundIntent Intent, ImmutableArray<Refinement> Refinements,
    PatchLocks Locks, ImmutableArray<string> Notices, bool HasSupportedRequest);
/// <summary>Deliberately limited vocabulary. Unknown clauses and conflicts reject the whole request.</summary>
public static class PhraseParser
{
    public static PhraseResult Parse(string prompt,SoundFamily defaultFamily,uint seed)
    {
        ArgumentNullException.ThrowIfNull(prompt);
        var remaining=prompt.ToLowerInvariant();
        // Preservation has an explicit grammar below; other negation is ambiguous.
        // Reject the entire request rather than turn a negative clause into a positive edit.
        if (Regex.IsMatch(remaining, @"\b(not|never|without|no|don't|dont|do\s+not|cannot|can't|avoid)\b"))
            return new(new(defaultFamily, seed, VariationAmount:0), [], PatchLocks.None,
                ["Negation is not supported by these offline recipes. Use explicit controls or preservation phrases such as 'keep the tuning'. No changes proposed."], false);
        var edits=ImmutableArray.CreateBuilder<Refinement>();var notices=ImmutableArray.CreateBuilder<string>();
        var families=new List<SoundFamily>();var groups=LockGroup.None;
        bool Take(string phrase)
        {
            string pattern=@"\b"+Regex.Escape(phrase)+@"\b";
            if(!Regex.IsMatch(remaining,pattern))return false;
            remaining=Regex.Replace(remaining,pattern," ");return true;
        }
        if(Take("keep its attack and tuning")|Take("keep the attack and tuning")|Take("keep attack and tuning"))
            groups|=LockGroup.Attack|LockGroup.Tuning;
        if(Take("keep the attack")|Take("keep attack")|Take("keep its attack"))groups|=LockGroup.Attack;
        if(Take("keep the tuning")|Take("keep tuning")|Take("keep pitch"))groups|=LockGroup.Tuning;
        if(Take("keep algorithm")|Take("keep the algorithm"))groups|=LockGroup.Algorithm;
        if(Take("keep envelopes")|Take("keep the envelopes"))groups|=LockGroup.Envelopes;
        (string Phrase,SoundFamily Family)[] names=[("bass",SoundFamily.Bass),("keys",SoundFamily.Keys),
            ("electric piano",SoundFamily.Keys),("bell",SoundFamily.Bell),("pad",SoundFamily.Pad),("brass",SoundFamily.Brass),
            ("organ",SoundFamily.Organ),("pluck",SoundFamily.Pluck),("percussion",SoundFamily.Percussion),("effect",SoundFamily.Effect)];
        foreach(var (phrase,family) in names)if(Take(phrase))families.Add(family);
        (string Phrase,RefinementDimension Dimension,int Amount)[] commands=[
            ("darker",RefinementDimension.Brightness,-50),("warmer",RefinementDimension.Brightness,-50),
            ("warm",RefinementDimension.Brightness,-50),("round",RefinementDimension.Brightness,-50),
            ("brighter",RefinementDimension.Brightness,50),("bright",RefinementDimension.Brightness,50),
            ("glassy",RefinementDimension.Brightness,50),("sharp attack",RefinementDimension.AttackSpeed,50),
            ("fast attack",RefinementDimension.AttackSpeed,50),("slow attack",RefinementDimension.AttackSpeed,-50),
            ("softer attack",RefinementDimension.AttackSpeed,-50),("shorter release",RefinementDimension.ReleaseLength,-50),
            ("longer release",RefinementDimension.ReleaseLength,50),("shorter decay",RefinementDimension.DecayLength,-50),
            ("longer decay",RefinementDimension.DecayLength,50),("shorter",RefinementDimension.DecayLength,-50),
            ("short",RefinementDimension.DecayLength,-50),("little sustain",RefinementDimension.Sustain,-50),
            ("more sustain",RefinementDimension.Sustain,50),("more expressive",RefinementDimension.VelocityResponse,50),
            ("less expressive",RefinementDimension.VelocityResponse,-50),
            ("brighter when i play harder",RefinementDimension.VelocityResponse,50),
            ("less motion",RefinementDimension.Movement,-50),("more movement",RefinementDimension.Movement,50),
            ("subtle motion",RefinementDimension.Movement,30),("metallic",RefinementDimension.Harmonicity,-50),
            ("inharmonic",RefinementDimension.Harmonicity,-50),("harmonic",RefinementDimension.Harmonicity,50)];
        // Longer phrases take precedence over substrings, e.g. velocity expression versus brightness.
        foreach(var command in commands.OrderByDescending(c=>c.Phrase.Length))
            if(Take(command.Phrase))edits.Add(new(command.Dimension,command.Amount));
        var register=Take("low register")?PlayingRegister.Low:Take("high register")?PlayingRegister.High:PlayingRegister.Middle;
        var voice=Take("mono")?VoicePreference.Mono:Take("poly")?VoicePreference.Poly:VoicePreference.Template;
        if(families.Distinct().Count()>1)notices.Add("Multiple families requested; no changes proposed.");
        foreach(var group in edits.GroupBy(e=>e.Dimension))if(group.Any(e=>e.Amount<0)&&group.Any(e=>e.Amount>0))
            notices.Add($"Conflicting {group.Key} requests; no changes proposed.");
        int Goal(RefinementDimension dimension)
        {
            var e=edits.Where(e=>e.Dimension==dimension).ToArray();
            return e.Any(x=>x.Amount<0)&&e.Any(x=>x.Amount>0)?0:Math.Sign(e.Sum(x=>x.Amount));
        }
        remaining=Regex.Replace(remaining,@"\b(make|this|patch|a|an|the|with|and|but|its|sound|it|that becomes)\b"," ");
        remaining=Regex.Replace(remaining,@"[^a-z0-9]+"," ").Trim();
        if(remaining.Length>0)notices.Add($"Unsupported or unrecognized wording: '{remaining}'. No changes proposed.");
        if(remaining.Length>0 || notices.Count>0)
            return new(new(defaultFamily,seed,VariationAmount:0),[],PatchLocks.None,notices.ToImmutable(),false);
        var intent=new SoundIntent(families.Distinct().Count()==1?families[0]:defaultFamily,seed,
            (Character)Goal(RefinementDimension.Brightness),(Character)Goal(RefinementDimension.Harmonicity),
            (Character)Goal(RefinementDimension.AttackSpeed),(Character)Goal(RefinementDimension.DecayLength),
            (Character)Goal(RefinementDimension.Sustain),(Character)Goal(RefinementDimension.ReleaseLength),
            (Character)Goal(RefinementDimension.VelocityResponse),(Character)Goal(RefinementDimension.Movement),register,voice);
        intent.Validate();
        return new(intent,edits.ToImmutable(),new(groups,ImmutableHashSet<int>.Empty),notices.ToImmutable(),
            edits.Count>0||families.Count>0||groups!=0||register!=PlayingRegister.Middle||voice!=VoicePreference.Template);
    }
}
