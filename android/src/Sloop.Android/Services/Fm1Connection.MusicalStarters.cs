using Sloop.Protocol;

namespace Sloop.Android.Services;
public sealed partial class Fm1Connection
{
    /// <summary>Lease maintenance shares EditorClient serialization without publishing UI busy state.</summary>
    public async Task<MusicalStarterStatus> MaintainMusicalStarterLeaseAsync(DeviceInfo identity, ushort lease, bool renew,
        CancellationToken cancellationToken = default)
    {
        var current=epoch;var link=client;
        if(current is null || link is null || current.IsCancellationRequested || !ReferenceEquals(Snapshot.Device,identity) || Snapshot.IsSimulated || Snapshot.IsGenericMidi)
            throw new IOException("Connection changed. Browse musical starters again.");
        // Normal dialog cancellation must not abandon an in-flight same-command reply and fault the epoch.
        cancellationToken.ThrowIfCancellationRequested();
        var result=await link.MusicalStarterLeaseAsync(lease,renew,current.Token);
        if(epoch!=current || !ReferenceEquals(Snapshot.Device,identity))throw new IOException("Connection changed during starter audition.");
        return result;
    }
}
