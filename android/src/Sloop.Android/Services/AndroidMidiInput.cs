using Android.Content;
using Android.Media.Midi;
using Android.OS;

namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    // Used only to exclude the destination from controller discovery (including generic MIDI).
    public int? MidiDestinationDeviceId=>deviceId;
    internal object? MidiDestinationSession=>epoch;
}

internal sealed record MidiInputCandidate(MidiDeviceInfo Device,int Port,string Name);

/// <summary>Receive-only adapter: never opens an Android input (send) port.</summary>
internal sealed class AndroidMidiInput : IDisposable
{
    readonly MidiManager manager; readonly Handler handler=new(Looper.MainLooper!);
    readonly DeviceEvents events; readonly Action<byte[]> receive; readonly Action<string> stopped;
    MidiDevice? device; MidiOutputPort? port; Receiver? receiver;
    int generation, pending, deliveryEpoch; bool disposed; int selectedId=-1;
    public AndroidMidiInput(Context context,Action<byte[]> receive,Action<string> stopped)
    {
        manager=context.GetSystemService(Context.MidiService) as MidiManager??throw new IOException("Android MIDI service unavailable.");
        this.receive=receive;this.stopped=stopped;events=new(this);
        if(OperatingSystem.IsAndroidVersionAtLeast(33))manager.RegisterDeviceCallback((int)MidiTransport.MidiByteStream,context.MainExecutor!,events);
        else manager.RegisterDeviceCallback(events,handler);
    }
    public MidiInputCandidate[] Candidates(int? excluded)
    {
        var devices=OperatingSystem.IsAndroidVersionAtLeast(33)?manager.GetDevicesForTransport((int)MidiTransport.MidiByteStream):manager.GetDevices();
        return (devices??[]).Where(d=>d.Id!=excluded).SelectMany(d=>(d.GetPorts()??[]).Where(p=>p.Type==MidiPortType.Output).Select(p=>new MidiInputCandidate(d,p.PortNumber,$"{d.Properties?.GetString(MidiDeviceInfo.PropertyName)??"MIDI device"} · port {p.PortNumber+1} {p.Name}"))).ToArray();
    }
    public void Open(MidiInputCandidate candidate,Action<string> ready)
    {
        Close();int epoch=generation;selectedId=candidate.Device.Id;
        manager.OpenDevice(candidate.Device,new OpenListener(this,candidate,epoch,ready),handler);
        handler.PostDelayed(()=>{if(!disposed&&generation==epoch&&device is null){Close();stopped("Controller open timed out.");}},5000);
    }
    void Accept(MidiDevice? opened,MidiInputCandidate candidate,int epoch,Action<string> ready)
    {
        if(disposed||epoch!=generation){opened?.Close();return;}
        try {
            device=opened??throw new IOException("Android could not open controller.");
            port=device.OpenOutputPort(candidate.Port)??throw new IOException("Controller output port unavailable.");
            receiver=new(this,epoch);port.Connect(receiver);ready(candidate.Name);
        }catch(Exception e){Close();stopped(e.Message);}
    }
    void Enqueue(byte[] bytes,int epoch)
    {
        int delivery=Volatile.Read(ref deliveryEpoch);
        if(Interlocked.Increment(ref pending)>64){Interlocked.Decrement(ref pending);handler.Post(()=>{if(generation==epoch){Close();stopped("Controller stopped: input queue overflow.");}});return;}
        handler.Post(()=>{Interlocked.Decrement(ref pending);if(!disposed&&generation==epoch&&delivery==deliveryEpoch)receive(bytes);});
    }
    public void DiscardPending()=>Interlocked.Increment(ref deliveryEpoch);
    public void Close()
    {
        generation++;DiscardPending();selectedId=-1;
        try{if(receiver is not null)port?.Disconnect(receiver);}catch(Java.IO.IOException){}
        try{port?.Close();}catch(Java.IO.IOException){}
        try{device?.Close();}catch(Java.IO.IOException){}
        receiver?.Dispose();receiver=null;port=null;device=null;
    }
    public void Dispose(){if(disposed)return;disposed=true;Close();manager.UnregisterDeviceCallback(events);events.Dispose();handler.Dispose();}
    sealed class Receiver(AndroidMidiInput owner,int epoch):MidiReceiver
    {
        public override void OnSend(byte[]? data,int offset,int count,long timestamp){if(data is not null&&count>0)owner.Enqueue(data.AsSpan(offset,count).ToArray(),epoch);}
        public override void OnFlush()=>owner.handler.Post(()=>{if(owner.generation==epoch){owner.Close();owner.stopped("Controller stream flushed; reconnect to resume.");}});
    }
    sealed class OpenListener(AndroidMidiInput owner,MidiInputCandidate candidate,int epoch,Action<string> ready):Java.Lang.Object,MidiManager.IOnDeviceOpenedListener
    {public void OnDeviceOpened(MidiDevice? device)=>owner.Accept(device,candidate,epoch,ready);}
    sealed class DeviceEvents(AndroidMidiInput owner):MidiManager.DeviceCallback
    {public override void OnDeviceRemoved(MidiDeviceInfo? info){if(info?.Id==owner.selectedId){owner.Close();owner.stopped("Controller disconnected.");}}}
}
