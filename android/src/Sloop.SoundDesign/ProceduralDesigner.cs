using System.Collections.Immutable;

namespace Sloop.SoundDesign;

/// <summary>Offline recipes only. Does no inference, synthesis, IO or hardware mutation.</summary>
public static class ProceduralDesigner
{
    public const string Version = "fm6-procedural/1";
    public static ImmutableArray<PatchDraft> Generate(string targetId,long revision,SoundState snapshot,
        SoundIntent intent,PatchLocks? locks=null,string prompt="",CancellationToken cancellationToken=default)
    {
        intent.Validate();PatchValidation.Require(snapshot.Patch);snapshot.Macros.Validate();locks??=PatchLocks.None;locks.Validate();
        ArgumentException.ThrowIfNullOrWhiteSpace(targetId);
        if(revision<0)throw new ArgumentOutOfRangeException(nameof(revision));
        cancellationToken.ThrowIfCancellationRequested();
        int index=intent.Family switch {SoundFamily.Keys=>0,SoundFamily.Bell=>1,SoundFamily.Bass=>2,
            SoundFamily.Brass=>3,SoundFamily.Pad=>4,SoundFamily.Percussion=>5,SoundFamily.Organ=>6,SoundFamily.Pluck=>7,_=>1};
        var template=FactoryLibrary.Get(index);
        var result=ImmutableArray.CreateBuilder<PatchDraft>(4);
        for(int variation=0;variation<4;variation++)
        {
            cancellationToken.ThrowIfCancellationRequested();
            uint seed=unchecked(intent.Seed + (uint)variation*0x9e3779b9u);
            var rng=new StableRandom(seed);var patch=locks.Enforce(snapshot.Patch,template);
            var goals=new[]{intent.Brightness,intent.AttackSpeed,intent.DecayLength,intent.Sustain,
                intent.ReleaseLength,intent.VelocityResponse,intent.Movement,intent.Harmonicity};
            var dimensions=new[]{RefinementDimension.Brightness,RefinementDimension.AttackSpeed,RefinementDimension.DecayLength,
                RefinementDimension.Sustain,RefinementDimension.ReleaseLength,RefinementDimension.VelocityResponse,
                RefinementDimension.Movement,RefinementDimension.Harmonicity};
            for(int d=0;d<goals.Length;d++) patch=Mutate(patch,new(dimensions[d],(int)goals[d]*50));
            // Variation touches spectral levels only; does not randomly retune carriers or change attack.
            for(int n=1;n<=6;n++)
            {
                var o=patch.GetOperator(n);
                if(o.OutputLevel==0) continue;
                int spread=AlgorithmTopology.IsCarrier(patch.Algorithm,n)?intent.VariationAmount/20:intent.VariationAmount/5;
                patch=patch.WithOperator(n,o with {OutputLevel=Clamp(o.OutputLevel+rng.Signed(spread),99)});
            }
            patch=locks.Enforce(snapshot.Patch,patch);
            var notes=ImmutableArray.CreateBuilder<string>();
            notes.Add($"Procedural variation {variation+1} from {template.Name}; envelope rates are firmware units, not milliseconds.");
            if(intent.Family==SoundFamily.Effect) notes.Add("Effect uses a glass-bell approximation; no effects processing is added.");
            if(intent.VoicePreference!=VoicePreference.Template) notes.Add($"{intent.VoicePreference} preference is metadata; voice allocation requires a track adapter (maximum six FM6 voices).");
            if(snapshot.Macros!=MacroContext.Neutral) notes.Add("Draft declares neutral sound macros; audition/apply must explicitly include this context.");
            result.Add(Build(targetId,revision,snapshot,new(patch,MacroContext.Neutral),intent,locks,seed,
                $"factory:{index}", $"{intent.Family} variation {variation+1}",prompt,notes.ToImmutable()));
        }
        return result.ToImmutable();
    }
    public static PatchDraft Refine(string targetId,long revision,SoundState snapshot,SoundIntent intent,
        IEnumerable<Refinement> refinements,PatchLocks? locks=null,string prompt="",CancellationToken cancellationToken=default)
    {
        intent.Validate();PatchValidation.Require(snapshot.Patch);snapshot.Macros.Validate();locks??=PatchLocks.None;locks.Validate();
        ArgumentException.ThrowIfNullOrWhiteSpace(targetId);
        if(revision<0)throw new ArgumentOutOfRangeException(nameof(revision));
        cancellationToken.ThrowIfCancellationRequested();
        var edits=refinements.ToImmutableArray();foreach(var e in edits)e.Validate();
        var explanation=ImmutableArray.CreateBuilder<string>();var patch=snapshot.Patch;
        foreach(var group in edits.GroupBy(e=>e.Dimension))
        {
            cancellationToken.ThrowIfCancellationRequested();
            if(group.Any(e=>e.Amount<0)&&group.Any(e=>e.Amount>0))
            {explanation.Add($"Conflicting {group.Key} requests left that dimension unchanged.");continue;}
            int amount=(int)Math.Clamp(group.Sum(e=>(long)e.Amount),-100,100);
            patch=Mutate(patch,new(group.Key,amount));
        }
        patch=locks.Enforce(snapshot.Patch,patch);
        if(PatchCodec.ContentEquals(patch,snapshot.Patch)) explanation.Add("No patch changes: request is neutral, saturated, conflicting, or blocked by locks/topology.");
        else explanation.Add("Bounded refinement of the current patch; unrelated dimensions and macro context preserved.");
        return Build(targetId,revision,snapshot,new(patch,snapshot.Macros),intent,locks,intent.Seed,
            "current-patch",snapshot.Patch.Name,prompt,explanation.ToImmutable()) with {Refinements=edits};
    }
    /// <summary>Refines a preview while retaining the original application/undo snapshot.</summary>
    public static PatchDraft RefineDraft(PatchDraft draft,IEnumerable<Refinement> edits,string prompt="",
        CancellationToken cancellationToken=default)
    {
        var next=Refine(draft.TargetId,draft.ParentRevision,draft.After,draft.Intent,edits,draft.Locks,prompt,cancellationToken);
        return next with {Before=draft.Before,Changes=Diff(draft.Before.Patch,next.After.Patch),
            BasePatchReference=draft.BasePatchReference,FullName=draft.FullName,ParentDraftIdentity=draft.Identity,
            Refinements=(draft.Refinements.IsDefault?ImmutableArray<Refinement>.Empty:draft.Refinements).AddRange(next.Refinements)};
    }
    static Patch Mutate(Patch p,Refinement edit)
    {
        if(edit.Amount==0)return p;
        int delta=(Math.Abs(edit.Amount)+9)/10*Math.Sign(edit.Amount);
        for(int n=1;n<=6;n++)
        {
            var o=p.GetOperator(n);if(o.OutputLevel==0)continue;
            var e=o.Envelope;bool carrier=AlgorithmTopology.IsCarrier(p.Algorithm,n);
            o=edit.Dimension switch
            {
                RefinementDimension.Brightness when !carrier=>o with {OutputLevel=Clamp(o.OutputLevel+delta,99)},
                RefinementDimension.AttackSpeed=>o with {Envelope=e with {Rate1=Clamp(e.Rate1+delta,99)}},
                RefinementDimension.DecayLength=>o with {Envelope=e with {Rate2=Clamp(e.Rate2-delta,99),Rate3=Clamp(e.Rate3-delta,99)}},
                RefinementDimension.Sustain=>o with {Envelope=e with {Level3=Clamp(e.Level3+delta,99)}},
                RefinementDimension.ReleaseLength=>o with {Envelope=e with {Rate4=Clamp(e.Rate4-delta,99),Level4=0}},
                RefinementDimension.VelocityResponse=>o with {VelocitySensitivity=Clamp(o.VelocitySensitivity+Math.Sign(delta),7)},
                RefinementDimension.Harmonicity when !carrier && o.Mode==FrequencyMode.Ratio=>o with
                    {Fine=edit.Amount>0?0:Clamp(o.Fine+delta*-3,99)},
                _=>o
            };
            p=p.WithOperator(n,o);
        }
        if(edit.Dimension==RefinementDimension.Movement) p=p with {LfoAmplitudeDepth=Clamp(p.LfoAmplitudeDepth+delta,99)};
        return p;
    }
    static int Clamp(int value,int max)=>Math.Clamp(value,0,max);
    static PatchDraft Build(string target,long revision,SoundState before,SoundState after,SoundIntent intent,
        PatchLocks locks,uint seed,string reference,string fullName,string prompt,ImmutableArray<string> explanation)
    {
        PatchValidation.Require(after.Patch);
        return new(target,revision,before,after,intent,locks,seed,Version,reference,fullName,prompt,explanation,
            Diff(before.Patch,after.Patch),Audition(intent));
    }
    public static ImmutableArray<PatchChange> Diff(Patch before,Patch after)
    {
        var a=PatchCodec.EncodeVoice(before);var b=PatchCodec.EncodeVoice(after);
        var changes=ImmutableArray.CreateBuilder<PatchChange>();
        string[] opFields=["rate1","rate2","rate3","rate4","level1","level2","level3","level4","breakpoint",
            "leftDepth","rightDepth","leftCurve","rightCurve","rateScaling","amplitudeSensitivity","velocitySensitivity",
            "outputLevel","mode","coarse","fine","detune"];
        string[] voiceFields=["pitch.rate1","pitch.rate2","pitch.rate3","pitch.rate4","pitch.level1","pitch.level2",
            "pitch.level3","pitch.level4","algorithm","feedback","oscillatorSync","lfoSpeed","lfoDelay",
            "lfoPitchDepth","lfoAmplitudeDepth","lfoSync","lfoWave","pitchSensitivity","transpose"];
        for(int i=0;i<a.Length;i++)if(a[i]!=b[i])changes.Add(new(i<126?$"OP{6-i/21}.{opFields[i%21]}":
            i<145?voiceFields[i-126]:$"name[{i-145}]",i==134?a[i]+1:a[i],i==134?b[i]+1:b[i]));
        return changes.ToImmutable();
    }
    static AuditionPlan Audition(SoundIntent intent)
    {
        int root=intent.Register switch {PlayingRegister.Low=>36,PlayingRegister.High=>72,_=>60};
        if(intent.Family==SoundFamily.Bass && intent.Register==PlayingRegister.Middle)root=36;
        int[] offsets=intent.Family is SoundFamily.Pad or SoundFamily.Brass or SoundFamily.Organ?[0,4,7]:[0,7,12];
        var notes=ImmutableArray.CreateBuilder<AuditionNote>();
        for(int velocity=0;velocity<3;velocity++)for(int n=0;n<3;n++)
            notes.Add(new(root+offsets[n],new[]{40,80,120}[velocity],velocity*24+(intent.Family==SoundFamily.Pad?0:n*4),
                intent.Family==SoundFamily.Pad?16:3));
        return new(100,notes.ToImmutable());
    }
    // Versioned xorshift32, stable across runtime versions. Zero is mapped explicitly.
    sealed class StableRandom(uint seed)
    {
        uint state=seed==0?0x6d2b79f5u:seed;
        public int Signed(int spread)
        {state^=state<<13;state^=state>>17;state^=state<<5;return (int)(state%(uint)(spread*2+1))-spread;}
    }
}
