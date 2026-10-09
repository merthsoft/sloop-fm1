using System.Collections.Immutable;
using Sloop.Protocol;
using Sloop.Sequencing;

namespace Sloop.Workstation;
public static class HardwarePatterns
{
    public static async Task<HardwarePattern> Read(EditorClient client,DeviceInfo info,HardwarePattern? identities,CancellationToken token)
    {
        if(info.ProtocolVersion<9||info.TrackCount!=4||info.StepCount!=64||info.EngineParameterStart!=53||info.ParameterCount!=61)
            throw new IOException("Native editing requires the current four-track SLOOP parameter layout.");
        var selected=await client.RequestAsync(27,[],cancellationToken:token);
        if(selected.Arguments.Length<2||selected.Arguments[0]>3||selected.Arguments[1]!=4)throw new IOException("Invalid track selection reply.");
        byte original=selected.Arguments[0];var tracks=ImmutableArray.CreateBuilder<HardwareTrack>(4);
        try {
            for(byte track=0;track<4;track++) {
                await client.RequestAsync(27,[track],f=>f.Arguments.Length>=2&&f.Arguments[0]==track,cancellationToken:token);
                var dump=await client.RequestAsync(29,[track],f=>f.Arguments.Length==125&&f.Arguments[0]==track,cancellationToken:token);
                var parameters=Enumerable.Range(0,61).ToImmutableDictionary(i=>i,i=>EditorCodec.DecodeValue(dump.Arguments.AsSpan(3+i*2,2)));
                var ranges=ImmutableDictionary.CreateBuilder<int,ParameterRange>();
                foreach(int id in HardwareCapabilities.CurrentFirmwareLockable) {var d=await client.GetDescriptorAsync(0,(byte)id,token);ranges[id]=new(d.Minimum,d.Maximum);}
                var old=identities?.Tracks[track];
                var micro=await client.RequestAsync(39,[track],f=>f.Arguments.Length==65&&f.Arguments[0]==track,cancellationToken:token);
                var fill=await client.RequestAsync(41,[track],f=>f.Arguments.Length==20&&f.Arguments[0]==track,cancellationToken:token);
                var conditions=EditorCodec.Unpack7(fill.Arguments.AsSpan(1));
                if(conditions.Length!=16)throw new IOException("Invalid fill snapshot.");
                var steps=ImmutableArray.CreateBuilder<HardwareStep>(64);
                for(byte step=0;step<64;step++) {
                    var identity=old?.Steps[step]??HardwareEditor.Blank(track==3);
                    var offset=new MicroOffset(micro.Arguments[step+1]-64);var condition=(FillCondition)((conditions[step/4]>>((step%4)*2))&3);
                    if(track==3) {
                        var reply=await client.RequestAsync(33,[step],f=>f.Arguments.Length==14&&f.Arguments[0]==step,cancellationToken:token);
                        steps.Add(HardwareSteps.DecodeDrum(reply.Arguments.AsSpan(1),(DrumStep)identity,offset,condition));
                    }else {
                        var reply=await client.RequestAsync(30,[track,step],f=>f.Arguments.Length==13&&f.Arguments[0]==track&&f.Arguments[1]==step,cancellationToken:token);
                        steps.Add(HardwareSteps.DecodeSynth(reply.Arguments.AsSpan(2),(SynthStep)identity,offset,condition));
                    }
                }
                var locks=await client.RequestAsync(37,[track],f=>f.Arguments.Length>=2&&f.Arguments[0]==track,cancellationToken:token);
                var a=locks.Arguments;
                if(a[1]>24||a.Length!=2+a[1]*4)throw new IOException("Invalid lock snapshot.");
                var events=ImmutableArray.CreateBuilder<ParameterLock>();
                for(int i=2;i<a.Length;i+=4)events.Add(new(old?.Locks.FirstOrDefault(l=>l.Step.Value==a[i]&&l.Parameter==a[i+1])?.Id??Guid.NewGuid(),new(a[i]),a[i+1],EditorCodec.DecodeValue(a.AsSpan(i+2,2))));
                tracks.Add(new(old?.Id??Guid.NewGuid(),track==3,parameters[29],steps.ToImmutable(),events.ToImmutable(),parameters,
                    new($"{info.Firmware}/engine:{dump.Arguments[1]}/preset:{dump.Arguments[2]}",HardwareCapabilities.CurrentFirmwareLockable,ranges.ToImmutable())));
            }
        }finally {try{await client.RequestAsync(27,[original],f=>f.Arguments.Length>=2&&f.Arguments[0]==original,cancellationToken:token);}catch(Exception){} }
        var result=new HardwarePattern(identities?.Id??Guid.NewGuid(),Guid.NewGuid(),tracks.ToImmutable());PatternValidation.Validate(result);return result;
    }
    public static async Task<HardwarePattern> Apply(EditorClient client,DeviceInfo info,EditProposal proposal,CancellationToken token)
    {
        if(proposal.Before is not HardwarePattern before||proposal.After is not HardwarePattern after)throw new ArgumentException("Expected native proposal.");
        PatternValidation.Validate(after);
        for(int track=0;track<4;track++)
            if(!before.Tracks[track].Parameters.OrderBy(p=>p.Key).SequenceEqual(after.Tracks[track].Parameters.OrderBy(p=>p.Key)))
                throw new IOException("Native parameter edits require their separate adapter.");
        var fresh=await Read(client,info,before,token);
        if(!fresh.Tracks.Zip(before.Tracks).All(p=>HardwareSteps.ContentEquals(p.First,p.Second)))throw new IOException("Hardware pattern changed since Read. Read again before applying.");
        // Reject unrepresentable writes before sending any part of the transaction.
        for(int track=0;track<4;track++)for(int step=0;step<64;step++)
            if(before.Tracks[track].Steps[step].Fill!=after.Tracks[track].Steps[step].Fill&&after.Tracks[track].Steps[step].Fill==FillCondition.ReservedNormal)
                throw new IOException("Reserved fill code cannot be written by this firmware.");
        for(byte track=0;track<4;track++) {
            var a=before.Tracks[track];var b=after.Tracks[track];
            for(byte step=0;step<64;step++) {
                var old=a.Steps[step];var next=b.Steps[step];var raw=HardwareSteps.Encode(next);
                if(!HardwareSteps.Encode(old).SequenceEqual(raw)) {
                    if(track==3) {var r=await client.RequestAsync(33,[step,..raw],f=>f.Arguments.Length==14&&f.Arguments[0]==step,cancellationToken:token);if(!r.Arguments.AsSpan(1).SequenceEqual(raw))throw new IOException("Drum acknowledgment differs.");}
                    else {var r=await client.RequestAsync(30,[track,step,..raw],f=>f.Arguments.Length==13&&f.Arguments[0]==track&&f.Arguments[1]==step,cancellationToken:token);if(!r.Arguments.AsSpan(2).SequenceEqual(raw))throw new IOException("Step acknowledgment differs.");}
                }
                if(old.Micro!=next.Micro){var r=await client.RequestAsync(40,[track,step,(byte)(next.Micro.Value+64)],cancellationToken:token);if(!r.Arguments.SequenceEqual(new byte[]{track,step,(byte)(next.Micro.Value+64)}))throw new IOException("Micro acknowledgment differs.");}
                if(old.Fill!=next.Fill){var r=await client.RequestAsync(42,[track,step,(byte)next.Fill],cancellationToken:token);if(!r.Arguments.SequenceEqual(new byte[]{track,step,(byte)next.Fill}))throw new IOException("Fill acknowledgment differs.");}
            }
            foreach(var old in a.Locks) if(!b.Locks.Any(l=>l.Step==old.Step&&l.Parameter==old.Parameter)) await Lock(client,track,old,false,token);
            foreach(var next in b.Locks) if(!a.Locks.Any(l=>l.Step==next.Step&&l.Parameter==next.Parameter&&l.Value==next.Value))await Lock(client,track,next,true,token);
        }
        var verified=await Read(client,info,after,token);
        if(!verified.Tracks.Zip(after.Tracks).All(p=>HardwareSteps.ContentEquals(p.First,p.Second)))throw new IOException("Native pattern readback differs. Partial application is possible; reconnect and read.");
        return verified;
    }
    static async Task Lock(EditorClient client,byte track,ParameterLock value,bool set,CancellationToken token)
    {
        byte[] args=set?[track,(byte)value.Step.Value,(byte)value.Parameter,..EditorCodec.EncodeValue(value.Value)]:[track,(byte)value.Step.Value,(byte)value.Parameter];
        var r=await client.RequestAsync(38,args,f=>f.Arguments.Length==7&&f.Arguments[0]==track&&f.Arguments[1]==value.Step.Value&&f.Arguments[2]==value.Parameter,cancellationToken:token);
        if(r.Arguments[3]!=0||r.Arguments[4]!=(set?1:0)||set&&EditorCodec.DecodeValue(r.Arguments.AsSpan(5))!=value.Value)throw new IOException("Parameter lock acknowledgment differs.");
    }
}
