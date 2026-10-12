using System.Buffers.Binary;
using System.Text;
using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
using Sloop.Workstation;
using Sloop.Protocol;
int checks=0;
void Check(bool c,string m){if(!c)throw new Exception(m);checks++;}
void Reject(Action a,string m){try{a();}catch(ArgumentException){checks++;return;}throw new Exception(m);}
async Task Fail(Func<Task> a,string m){try{await a();}catch(IOException){checks++;return;}throw new Exception(m);}
var folder=Path.Combine(Path.GetTempPath(),"sloop-quick-samples-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(folder);
try {
    string path=Path.Combine(folder,"song.wav");
    var audio=Enumerable.Range(0,8000*32).Select(i=>(short)(12000*Math.Sin(2*Math.PI*440*i/8000))).ToArray();
    SamplePreparation.WritePreview(path,audio,8000);var source=PcmWave.Open(path);var doc=new SampleDocument(source);
    doc.TrimAndEqualParts(0,8000*8,4);
    var before=File.ReadAllBytes(path);var markers=doc.Markers.ToArray();long revision=doc.Revision;
    var options=new KitSettings("MEOW",SampleMapping.Instrument,60,MonoChoice.AverageChannels,1);
    var sound=QuickSampling.Build(doc,options,true);
    Check(sound.Artifact.Preview.Count==1,"Playable sound collapses the derivative, not the saved cuts.");
    Check(sound.Artifact.Preview[0].LowNote==0&&sound.Artifact.Preview[0].HighNote==127&&sound.Artifact.Preview[0].RootNote==60,"Sound spans the piano with original pitch at middle C.");
    Check(sound.Artifact.Preview[0].SampleRate==11025,"Eight-second phrase chooses highest fitting native rate.");
    Check(Math.Abs(sound.Artifact.Preview[0].DecodedPcm.Length/(double)sound.Artifact.Preview[0].SampleRate-8)<.001,"Lower rate preserves eight-second duration.");
    int crossings=0;var decoded=sound.Artifact.Preview[0].DecodedPcm;for(int i=1;i<decoded.Length;i++)if(decoded[i-1]<=0&&decoded[i]>0)crossings++;
    Check(Math.Abs(crossings/8.0-440)<2,"Decoded 440 Hz tone retains its pitch at the native playback rate.");
    Check(doc.Revision==revision&&doc.Markers.SequenceEqual(markers)&&File.ReadAllBytes(path).SequenceEqual(before),"Source recording and existing chops are untouched by sound preparation.");
    var chops=QuickSampling.Build(doc,options with{RootNote=48},false);
    Check(chops.Artifact.Preview.Count==4,"One bar/four bars both yield four equal chops.");
    Check(chops.Artifact.Preview.Select(z=>z.RootNote).SequenceEqual(new[]{48,49,50,51})&&chops.Artifact.Preview.All(z=>z.LowNote==z.RootNote&&z.HighNote==z.RootNote),"Chops trigger on consecutive distinct keys, with no accidental keyboard spreading.");
    Check(chops.Artifact.Preview.All(z=>z.SampleRate==11025&&Math.Abs(z.DecodedPcm.Length/(double)z.SampleRate-2)<.001),"Four two-second chops keep their length at reduced rate.");
    var restored=SlotImages.Read(chops.Artifact.Image);
    Check(restored.Image.SequenceEqual(chops.Artifact.Image)&&restored.Preview.All(z=>z.SampleRate==11025),"Slot readback retains native-rate metadata.");
    Check(BinaryPrimitives.ReadUInt32LittleEndian(chops.Artifact.Header.AsSpan(48))==16384,"Wire playback ratio is 11025/44100, not the default 22050/44100.");
    string previewPath=Path.Combine(folder,"preview.wav");SamplePreparation.WritePreview(previewPath,decoded,11025);
    Check(PcmWave.Open(previewPath).SampleRate==11025,"Phone encoded preview uses the same native rate.");
    doc.TrimAndEqualParts(100,8000*8-100,4);
    Check(doc.Slices().All(s=>s.End>s.Start)&&QuickSampling.EqualCount(doc)==4,"Dragging trim retains four evenly divided nonempty chops.");
    Check(doc.Undo()&&doc.Start==0&&doc.End==8000*8&&doc.Markers.SequenceEqual(markers),"One Undo restores both trim and equal chop boundaries.");
    Reject(()=>doc.TrimAndEqualParts(0,3,4),"Invalid short trim rejects before changes.");
    Check(doc.Start==0&&doc.End==8000*8&&doc.Markers.SequenceEqual(markers),"Rejected trim leaves all cuts intact.");
    doc.Split(1000);Check(QuickSampling.EqualCount(doc) is null,"Manual cuts are not mislabeled as equal chopping.");
    doc.TrimAndEqualParts(0,8000*2,4);Check(QuickSampling.FittingRate(doc)==22050,"Short recordings retain full default quality.");
    doc.TrimAndEqualParts(0,8000*16,4);Check(QuickSampling.FittingRate(doc)==8269,"Sixteen-second four-bar phrase fits without shortening.");
    doc.TrimAndEqualParts(0,8000*24,4);Check(QuickSampling.FittingRate(doc)==5513,"Longer phrases use the final supported quality before refusing data loss.");
    foreach(int seconds in new[]{16,24}) {
        doc.TrimAndEqualParts(0,8000*seconds,4);
        var longKit=QuickSampling.Build(doc,options with{RootNote=48},false);
        double firmwareSeconds=0;
        for(int zone=0;zone<4;zone++) {
            int p=32+zone*28;uint rate=BinaryPrimitives.ReadUInt32LittleEndian(longKit.Artifact.Header.AsSpan(p+16));
            uint n=BinaryPrimitives.ReadUInt32LittleEndian(longKit.Artifact.Header.AsSpan(p+4));
            double effectiveRate=44100.0*(rate>>8)/256;
            firmwareSeconds+=n/effectiveRate;
            Check(Math.Abs(effectiveRate-longKit.Artifact.Preview[zone].SampleRate)<.6,"Guided rate aligns with the actual firmware's rate>>8 playback stepping.");
        }
        Check(Math.Abs(firmwareSeconds-seconds)<.005,"Native playback retains long phrase timing within five milliseconds, not a drifted whole song beat.");
        Check(SlotImages.Read(longKit.Artifact.Image).Preview[0].SampleRate==longKit.Artifact.Preview[0].SampleRate,"Low-rate preview metadata survives readback rounding.");
    }
    doc.TrimAndEqualParts(0,8000*32,4);Reject(()=>QuickSampling.FittingRate(doc),"Overlong recording refuses instead of silently dropping chops.");
    doc.TrimAndEqualParts(0,8000,4);
    var edits=new[]{new ChopAudio(0,2000,RootNote:99)};
    var mapped=QuickSampling.Build(doc,options with{RootNote=48},false,edits);
    Check(mapped.Artifact.Preview[0].RootNote==48,"Simple mapping stays predictable even with an older advanced root override.");
    var cancelled=new CancellationTokenSource();cancelled.Cancel();try{QuickSampling.Build(doc,options,false,token:cancelled.Token);throw new Exception("Cancelled conversion continued.");}catch(OperationCanceledException){checks++;}
    Check(PcmConversion.MapBoundary(8000,8000,11025)==11025,"Loop/frame mapping uses the chosen output rate.");
    Reject(()=>PcmConversion.Convert([0.1],1,8000,new(MonoChoice.AverageChannels,1),targetRate:0),"Invalid target rate is rejected.");
    var info=new DeviceInfo("test",3,71,40,64,63,new[]{"ANALOG","SAMPLE","FM6"},4,14);
    var wire=new SampleWire();using(var client=new EditorClient(wire))await Fm1Operations.ActivateSampleSlot(client,info,2,1,default);
    Check(wire.Selected==1&&wire.Engine==1&&wire.Set==5,"Sample engine and USR3 are chosen from negotiated names, not fixed engine/set IDs.");
    Check(wire.Commands.SequenceEqual(new byte[]{27,8,5,31,29}),"Instrument activation includes acknowledged set write and complete readback.");
    wire=new SampleWire{WrongReadback=true};using(var client=new EditorClient(wire))await Fail(()=>Fm1Operations.ActivateSampleSlot(client,info,0,0,default),"Wrong instrument readback cannot report ready.");
    wire=new SampleWire{MissingSlot=true};using(var client=new EditorClient(wire))await Fail(()=>Fm1Operations.ActivateSampleSlot(client,info,0,0,default),"Missing slot choice rejects before SET write.");
    Check(!wire.Commands.Contains((byte)31),"Invalid descriptor does not write a guessed slot.");
    wire=new SampleWire();using(var client=new EditorClient(wire)){
        try{await Fm1Operations.ActivateSampleSlot(client,info with{ProtocolVersion=8},0,0,default);throw new Exception("Unsupported layout accepted.");}catch(ArgumentException){checks++;}
    }
    Check(wire.Commands.Count==0,"Unsupported activation rejects before any instrument changes.");
} finally {Directory.Delete(folder,true);}
Console.WriteLine($"Quick sampling: {checks} workflow, native-rate, trim/undo and activation checks passed.");

sealed class SampleWire:IMidiTransport {
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected {add{}remove{}}
    public readonly List<byte> Commands=[];public byte Selected,Engine;public int Set;
    public bool WrongReadback,MissingSlot;
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes,CancellationToken token){
        token.ThrowIfCancellationRequested();byte command=bytes.Span[4];var args=bytes.Span[5..^1].ToArray();Commands.Add(command);byte[] reply;
        switch(command){
            case 27:Selected=args[0];reply=[Selected,4];break;
            case 8:Engine=args[0];reply=[Engine,0];break;
            case 5:
                var d=new List<byte>{0,63,8};d.AddRange(EditorCodec.EncodeValue(0));d.AddRange(EditorCodec.EncodeValue(6));d.AddRange(EditorCodec.EncodeValue(0));
                foreach(string s in new[]{"SET","","A","B","C",MissingSlot?"NONE":"USR1","USR2","USR3","USR4"}){d.AddRange(Encoding.ASCII.GetBytes(s));d.Add(0);}reply=d.ToArray();break;
            case 31:Set=EditorCodec.DecodeValue(args.AsSpan(2));reply=[args[0],args[1],..EditorCodec.EncodeValue(Set)];break;
            case 29:var all=new List<byte>{Selected,Engine,0};for(int i=0;i<71;i++)all.AddRange(EditorCodec.EncodeValue(i==63?(WrongReadback?0:Set):0));reply=all.ToArray();break;
            default:throw new IOException("Unexpected activation command.");
        }
        BytesReceived?.Invoke(EditorCodec.Encode(command,reply));return ValueTask.CompletedTask;
    }
}
