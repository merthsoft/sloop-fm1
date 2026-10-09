using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.SoundDesign;

namespace Sloop.Workstation;

/// <summary>Bounded, versioned local files; atomic replacement, no reflection serializer.</summary>
public static class WorkspaceFiles
{
    public static void StoreBytes(string path,ReadOnlySpan<byte> bytes)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using(var stream=File.Create(path+".tmp")){stream.Write(bytes);stream.Flush(true);}
        File.Move(path+".tmp",path,true);
    }
    public static void SaveHardware(string path,HardwarePattern p)
    {
        PatternValidation.Validate(p);
        Write(path,w=> {
            w.Write("SLOOP-NATIVE-1");w.Write(p.Id.ToByteArray());w.Write(p.Revision.ToByteArray());
            foreach(var t in p.Tracks) {
                w.Write(t.Id.ToByteArray());w.Write(t.IsDrum);w.Write(t.Length);w.Write(t.Capabilities.Identity);
                w.Write(t.Parameters.Count);foreach(var pair in t.Parameters.OrderBy(p=>p.Key)){w.Write(pair.Key);w.Write(pair.Value);}
                w.Write(t.Capabilities.ParameterRanges.Count);foreach(var pair in t.Capabilities.ParameterRanges.OrderBy(p=>p.Key)){w.Write(pair.Key);w.Write(pair.Value.Minimum);w.Write(pair.Value.Maximum);w.Write(t.Capabilities.LockableParameters.Contains(pair.Key));}
                foreach(var s in t.Steps) {
                    w.Write(s.Id.ToByteArray());w.Write(s.Micro.Value);w.Write((int)s.Fill);
                    foreach(var id in s is SynthStep a?a.Slots.Select(n=>n.Id):((DrumStep)s).Lanes.Select(n=>n.Id))w.Write(id.ToByteArray());
                    w.Write(HardwareSteps.Encode(s));
                }
                w.Write(t.Locks.Length);foreach(var l in t.Locks){w.Write(l.Id.ToByteArray());w.Write(l.Step.Value);w.Write(l.Parameter);w.Write(l.Value);}
            }
        });
    }
    public static HardwarePattern LoadHardware(string path)
    {
        using var r=new BinaryReader(File.OpenRead(path));
        if(r.ReadString()!="SLOOP-NATIVE-1")throw new FormatException("Unknown native pattern file.");
        var id=new Guid(r.ReadBytes(16));var revision=new Guid(r.ReadBytes(16));var tracks=ImmutableArray.CreateBuilder<HardwareTrack>(4);
        int Count(int maximum){int n=r.ReadInt32();if(n<0||n>maximum)throw new FormatException("Invalid native collection size.");return n;}
        for(int track=0;track<4;track++) {
            var tid=new Guid(r.ReadBytes(16));bool drum=r.ReadBoolean();int length=r.ReadInt32();string identity=r.ReadString();
            var parameters=ImmutableDictionary.CreateBuilder<int,int>();int count=Count(61);for(int i=0;i<count;i++)parameters.Add(r.ReadInt32(),r.ReadInt32());
            var ranges=ImmutableDictionary.CreateBuilder<int,ParameterRange>();var lockable=ImmutableHashSet.CreateBuilder<int>();count=Count(61);
            for(int i=0;i<count;i++){int key=r.ReadInt32();ranges.Add(key,new(r.ReadInt32(),r.ReadInt32()));if(r.ReadBoolean())lockable.Add(key);}
            var steps=ImmutableArray.CreateBuilder<HardwareStep>(64);
            for(int step=0;step<64;step++) {
                var sid=new Guid(r.ReadBytes(16));var micro=new MicroOffset(r.ReadInt32());var fill=(FillCondition)r.ReadInt32();
                var ids=Enumerable.Range(0,drum?16:4).Select(_=>new Guid(r.ReadBytes(16))).ToArray();
                if(drum)steps.Add(HardwareSteps.DecodeDrum(r.ReadBytes(13),new(sid,micro,fill,ids.Select(i=>new DrumHit(i,false,HitLevel.Normal,1)).ToImmutableArray()),micro,fill));
                else steps.Add(HardwareSteps.DecodeSynth(r.ReadBytes(11),new(sid,micro,fill,ids.Select(i=>new SynthSlot(i,0,HitLevel.Normal,1)).ToImmutableArray(),0,StepTime.Rest,StepFlags.None,0),micro,fill));
            }
            var locks=ImmutableArray.CreateBuilder<ParameterLock>();count=Count(24);for(int i=0;i<count;i++)locks.Add(new(new(r.ReadBytes(16)),new(r.ReadInt32()),r.ReadInt32(),r.ReadInt32()));
            tracks.Add(new(tid,drum,length,steps.ToImmutable(),locks.ToImmutable(),parameters.ToImmutable(),new(identity,lockable.ToImmutable(),ranges.ToImmutable())));
        }
        var result=new HardwarePattern(id,revision,tracks.ToImmutable());PatternValidation.Validate(result);
        if(r.BaseStream.Position!=r.BaseStream.Length)throw new FormatException("Unexpected native pattern data.");return result;
    }
    static void Write(string path, Action<BinaryWriter> write)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using (var stream = File.Create(path + ".tmp")) {
            using var writer = new BinaryWriter(stream, System.Text.Encoding.UTF8, true);
            write(writer); writer.Flush(); stream.Flush(true);
        }
        File.Move(path + ".tmp", path, true);
    }
    public static void SaveSound(string path, SoundDocument doc) {
        var draft=doc.AcceptedHistory.LastOrDefault();
        if(draft is not null) {
            var proposalPath=path+".proposal-"+draft.Identity;
            if(!File.Exists(proposalPath))Write(proposalPath,w=> {
                w.Write("SLOOP-PROPOSAL-1");w.Write(draft.TargetId);w.Write(draft.ParentRevision);w.Write(draft.Seed);
                w.Write(draft.RecipeIdentity);w.Write(draft.BasePatchReference);w.Write(draft.FullName);w.Write(draft.Prompt);
                w.Write(PatchCodec.EncodeVoice(draft.Before.Patch));w.Write(PatchCodec.EncodeVoice(draft.After.Patch));
                foreach(var m in new[]{draft.Before.Macros,draft.After.Macros})foreach(var v in new[]{m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune})w.Write(v);
                w.Write((int)draft.Locks.Groups);w.Write(draft.Locks.Operators.Count);foreach(int op in draft.Locks.Operators.Order())w.Write(op);
                w.Write(draft.ParentDraftIdentity??"");w.Write(draft.Explanation.Length);foreach(string line in draft.Explanation)w.Write(line);
                var i=draft.Intent;foreach(int v in new[]{(int)i.Family,(int)i.Brightness,(int)i.Harmonicity,(int)i.AttackSpeed,(int)i.DecayLength,(int)i.Sustain,(int)i.ReleaseLength,(int)i.VelocityResponse,(int)i.Movement,(int)i.Register,(int)i.VoicePreference,i.VariationAmount})w.Write(v);
                w.Write(draft.Refinements.IsDefault?0:draft.Refinements.Length);if(!draft.Refinements.IsDefault)foreach(var e in draft.Refinements){w.Write((int)e.Dimension);w.Write(e.Amount);}
            });
        }
        Write(path, w => {
        w.Write("SLOOP-SOUND-1"); w.Write(doc.Revision); w.Write(PatchCodec.EncodeVoice(doc.State.Patch));
        var m=doc.State.Macros;
        foreach(var v in new[]{m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune}) w.Write(v);
        });
    }
    public static SoundDocument LoadSound(string path,string target)
    {
        using var r=new BinaryReader(File.OpenRead(path));
        if(r.ReadString()!="SLOOP-SOUND-1") throw new FormatException("Unknown sound file.");
        long revision=r.ReadInt64(); var patch=PatchCodec.DecodeVoice(r.ReadBytes(155));
        var m=new MacroContext(r.ReadInt32(),r.ReadInt32(),r.ReadInt32(),r.ReadInt32(),r.ReadInt32(),r.ReadInt32(),r.ReadInt32());
        if(r.BaseStream.Position!=r.BaseStream.Length) throw new FormatException("Unexpected sound data.");
        return new(target,new(patch,m),revision);
    }
    public static void SavePattern(string path,AppPattern pattern)
    {
        PatternValidation.Validate(pattern);
        if(pattern.Notes.Length>8192) throw new ArgumentException("Maximum 8192 app notes.");
        Write(path,w=> {
            w.Write("SLOOP-PATTERN-1"); w.Write(pattern.Id.ToByteArray()); w.Write(pattern.Revision.ToByteArray());
            w.Write(pattern.TicksPerQuarter); w.Write(pattern.Length.Value); w.Write(pattern.Notes.Length);
            foreach(var n in pattern.Notes) {
                w.Write(n.Id.ToByteArray()); w.Write(n.PartId.ToByteArray()); w.Write(n.Start.Value); w.Write(n.Duration.Value);
                w.Write(n.Pitch); w.Write(n.Velocity); w.Write(n.Channel);
            }
        });
    }
    public static AppPattern LoadPattern(string path)
    {
        using var r=new BinaryReader(File.OpenRead(path));
        if(r.ReadString()!="SLOOP-PATTERN-1") throw new FormatException("Unknown pattern file.");
        var id=new Guid(r.ReadBytes(16)); var revision=new Guid(r.ReadBytes(16));
        int ppq=r.ReadInt32(); long length=r.ReadInt64(); int count=r.ReadInt32();
        if(count is <0 or >8192) throw new FormatException("Invalid note count.");
        var notes=ImmutableArray.CreateBuilder<AppNote>(count);
        for(int i=0;i<count;i++) notes.Add(new(new(r.ReadBytes(16)),new(r.ReadBytes(16)),new(r.ReadInt64()),new(r.ReadInt64()),r.ReadInt32(),r.ReadInt32(),r.ReadInt32()));
        var result=new AppPattern(id,revision,ppq,new(length),notes.ToImmutable()); PatternValidation.Validate(result);
        if(r.BaseStream.Position!=r.BaseStream.Length) throw new FormatException("Unexpected pattern data.");
        return result;
    }
}
