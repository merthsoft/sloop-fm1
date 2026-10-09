using Sloop.Protocol;
using Sloop.Sequencing;
using Sloop.SoundDesign;
using Sloop.Workstation;

namespace Sloop.Android.Services;
public sealed partial class Fm1Connection
{
    void ValidateSceneTracks(IEnumerable<int> tracks)
    {
        var info=Snapshot.Device??throw new IOException("Connect a physical FM1 first.");
        if(info.ProtocolVersion<9||info.EngineParameterStart!=53||info.ParameterCount!=61||!info.Engines.Contains("FM6"))
            throw new IOException("Unsupported FM6 scene capabilities or parameter layout.");
        var values=tracks.ToArray();
        if(values.Length==0||values.Distinct().Count()!=values.Length||values.Any(t=>t<0||t>2||t>=info.TrackCount))
            throw new ArgumentException("Map each scene sound to a distinct FM6 synth track (0–2).");
    }
    public async Task<SceneHardwareBaseline> ReadSceneSoundsAsync(IEnumerable<int> tracks,CancellationToken cancellationToken=default)
    {
        var values=tracks.ToArray(); ValidateSceneTracks(values);
        return await EditDeviceAsync("Reading scene hardware baselines…",async(c,t)=> {
            using var linked=CancellationTokenSource.CreateLinkedTokenSource(t,cancellationToken);
            var states=new List<SceneTrackSound>();
            foreach(var track in values) { var s=await ReadSound(c,track,linked.Token); states.Add(new(track,s.State,s.Preset,s.Selector)); }
            foreach(var state in states) { var s=await ReadSound(c,state.Track,linked.Token);
                if(!SceneHardwareBatch.Same(state,new(state.Track,s.State,s.Preset,s.Selector)))throw new IOException("Hardware changed during batch Read. Read again."); }
            return new SceneHardwareBaseline(epoch!,states);
        });
    }
    public async Task<SceneHardwareResult> ApplySceneSoundsAsync(SceneHardwareBaseline baseline,
        IEnumerable<SceneHardwareTarget> targets,CancellationToken cancellationToken=default)
    {
        ArgumentNullException.ThrowIfNull(baseline);
        var values=targets.ToArray(); ValidateSceneTracks(values.Select(v=>v.Track));
        if(!ReferenceEquals(baseline.Connection,epoch)||!values.Select(v=>v.Track).Order().SequenceEqual(baseline.Tracks.Select(v=>v.Track).Order()))
            throw new IOException("Read a baseline for these exact tracks on this connection first.");
        foreach(var v in values) { v.State.Macros.Validate(); _=PatchCodec.Pack(v.State.Patch); }
        return await RunSceneBatchAsync(async(c,t)=> {
            var current=epoch; var start=Snapshot.Device!.EngineParameterStart;
            using var linked=CancellationTokenSource.CreateLinkedTokenSource(t,cancellationToken);
            void Guard() { t.ThrowIfCancellationRequested();
                if(epoch!=current||IsPlaying||IsRecording||Performer.Active)throw new IOException("Scene hardware requires stopped playback and released performance."); }
            async Task<SceneTrackSound> Read(int track) {
                var s=await ReadSound(c,track,t); Guard(); var next=await ReadSound(c,track,t);
                var first=new SceneTrackSound(track,s.State,s.Preset,s.Selector);
                var second=new SceneTrackSound(track,next.State,next.Preset,next.Selector);
                if(!SceneHardwareBatch.Same(first,second))throw new IOException("Hardware changed during complete sound readback.");
                return second;
            }
            async Task Write(SceneHardwareTarget v) {
                Guard(); linked.Token.ThrowIfCancellationRequested(); await Fm1Operations.WritePatch(c,(byte)v.Track,v.State.Patch,linked.Token);
                var m=v.State.Macros; int[] macros=[m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune];
                for(int i=0;i<macros.Length;i++) { Guard(); linked.Token.ThrowIfCancellationRequested();
                    if(await c.TrackParameterAsync((byte)v.Track,(byte)(start+i),macros[i],linked.Token)!=macros[i])throw new IOException("Macro acknowledgment differs."); }
            }
            return await SceneHardwareBatch.Apply(baseline.Tracks,values,Read,Write,Guard,()=>linked.Token.ThrowIfCancellationRequested());
        });
    }
    async Task<SceneHardwareResult> RunSceneBatchAsync(Func<EditorClient,CancellationToken,Task<SceneHardwareResult>> operation)
    {
        if(IsPlaying||IsRecording||Performer.Active||Snapshot.Busy||client is null||epoch is null||Snapshot.IsSimulated)
            throw new IOException("Connect a physical FM1, stop playback/recording and release performance first.");
        var current=epoch; var editor=client; var previous=Snapshot.Status;
        Publish(Snapshot with {Busy=true,Status="Applying scene hardware and reconciling tracks…"});
        // Unlike single-sound editing, preserve the per-track report across disconnect.
        try {
            await ClearHardwarePerformanceAsync();
            if (epoch != current) throw new IOException("Connection changed before scene operation.");
            return await operation(editor,current.Token);
        }
        finally { if(epoch==current)Publish(Snapshot with {Busy=false,Status=previous}); }
    }

    private readonly Dictionary<int,(int Preset,int Selector)> soundFingerprints=new();
    public async Task<T> EditDeviceAsync<T>(string status,Func<EditorClient,CancellationToken,Task<T>> operation)
    {
        if(IsPlaying||IsRecording||Performer.Active||Snapshot.Busy||client is null||epoch is null||Snapshot.IsSimulated) throw new IOException("Connect a physical FM1 and release performance/MIDI preview first.");
        var current=epoch; var editor=client; var previous=Snapshot.Status;
        Publish(Snapshot with{Busy=true,Status=status});
        try {
            await ClearHardwarePerformanceAsync();
            if (epoch != current) throw new IOException("Connection changed before device operation.");
            var result=await operation(editor,current.Token);
            if(epoch!=current) throw new IOException("Connection changed during operation.");
            Publish(Snapshot with{Busy=false,Status=previous}); return result;
        } catch(Exception e) {if(epoch==current)Fail(e.Message+" Reconnect and read back hardware state."); throw;}
    }
    public async Task<SoundState> ReadSoundAsync(int track)
    {
        if(Snapshot.Device?.ProtocolVersion<9) throw new IOException("FM6 requires editor protocol 9.");
        return await EditDeviceAsync("Reading FM6 patch and macros…",async(c,t)=> {
            var value=await ReadSound(c,track,t);soundFingerprints[track]=(value.Preset,value.Selector);return value.State;
        });
    }
    async Task<(SoundState State,int Preset,int Selector)> ReadSound(EditorClient c,int track,CancellationToken token)
    {
        var info=Snapshot.Device??throw new IOException("Device information unavailable.");
        var d=await c.RequestAsync(29,[(byte)track],f=>f.Arguments.Length>=3&&f.Arguments[0]==track,cancellationToken:token);
        if(d.Arguments.Length!=3+info.ParameterCount*2 || d.Arguments[1]>=info.Engines.Count || info.Engines[d.Arguments[1]]!="FM6") throw new IOException("Select FM6 on this hardware track before reading/applying patches.");
        int Value(int id)=>EditorCodec.DecodeValue(d.Arguments.AsSpan(3+id*2,2));
        int start=info.EngineParameterStart;
        var macros=new MacroContext(Value(start),Value(start+1),Value(start+2),Value(start+3),Value(start+4),Value(start+5),Value(start+6)); macros.Validate();
        return (new(await Fm1Operations.ReadPatch(c,(byte)track,token),macros),d.Arguments[2],Value(start+7));
    }
    public async Task<SoundState> ApplySoundAsync(int track,SoundState expected,SoundState desired)
    {
        return await EditDeviceAsync("Applying FM6 patch and checking readback…",async(c,t)=> {
            var fresh=await ReadSound(c,track,t);var actual=fresh.State;
            if(!soundFingerprints.TryGetValue(track,out var captured)||captured!=(fresh.Preset,fresh.Selector)||actual.Macros!=expected.Macros||!PatchCodec.ContentEquals(actual.Patch,expected.Patch)) throw new IOException("Hardware sound, preset or PTCH changed since Read. Read again before applying.");
            await Fm1Operations.WritePatch(c,(byte)track,desired.Patch,t);
            var m=desired.Macros; int[] values=[m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune];
            for(int i=0;i<7;i++) if(await c.TrackParameterAsync((byte)track,(byte)(Snapshot.Device!.EngineParameterStart+i),values[i],t)!=values[i]) throw new IOException("Macro acknowledgment differs.");
            var verified=await ReadSound(c,track,t);var readback=verified.State;
            if((verified.Preset,verified.Selector)!=captured||readback.Macros!=desired.Macros||!PatchCodec.ContentEquals(readback.Patch,desired.Patch)) throw new IOException("Sound readback differs.");
            return readback;
        });
    }
    private CancellationTokenSource? playback;
    private readonly object playbackSync=new();
    private readonly HashSet<(int Channel,int Pitch)> performanceNotes=[];
    public bool IsPlaying=>playback is not null;
    public void StopPlaying()
    {
        TransportStopping?.Invoke();
        lock(playbackSync) {
            playback?.Cancel();
            if(transport is IMidiTransport midi)foreach(var key in performanceNotes.ToArray())
                try{midi.SendAsync(new byte[]{(byte)(0x80+key.Channel),(byte)key.Pitch,0},CancellationToken.None).GetAwaiter().GetResult();}catch(Exception){}
            performanceNotes.Clear();
        }
    }
    // Compatibility preview remains a single phrase; app editor calls LoopPatternAsync explicitly.
    public async Task PlayNotesAsync(IEnumerable<AppNote> notes,int ppq,int tempo)
    {
        var values=notes.ToArray();long length=Math.Max(1,values.Select(n=>n.Start.Value+n.Duration.Value).DefaultIfEmpty(1).Max());
        var pattern=new AppPattern(Guid.NewGuid(),Guid.NewGuid(),ppq,new(length),System.Collections.Immutable.ImmutableArray.CreateRange(values));
        void StopAtEnd(TransportTick tick){if(tick.Loop>0)StopPlaying();}
        TransportTicked+=StopAtEnd;
        try{await LoopPatternAsync(pattern,tempo);}finally{TransportTicked-=StopAtEnd;}
    }
}

