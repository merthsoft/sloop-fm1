using Sloop.Workstation;
namespace Sloop.Android.Services;
public sealed partial class Fm1Connection
{
    PerformanceMacroPlayer? macros;
    public PerformanceMacroPlayer Macros {
        get {
            if(macros is null){macros=new(bytes=>{if(transport is Sloop.Protocol.IMidiTransport midi&&!Snapshot.IsSimulated) midi.SendAsync(bytes,CancellationToken.None).GetAwaiter().GetResult();});Changed+=()=>{if(!CanPerform){try{macros.Stop();}catch(Exception){macros.Forget();}}};}
            return macros;
        }
    }
}
