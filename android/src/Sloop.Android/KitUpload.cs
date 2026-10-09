using Android.Widget;
using Sloop.SampleEncoding;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private void ConfirmSlot(string title,string message,EventHandler<global::Android.Content.DialogClickEventArgs> choose)
    {
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle(title)!.SetMessage(message)!
            .SetPositiveButton("Choose slot",(_,_)=>new global::Android.App.AlertDialog.Builder(this)!.SetTitle(title)!
                .SetItems(new[]{"USR1","USR2","USR3","USR4"},choose)!.SetNegativeButton("Cancel",(_,_)=>{})!.Show())!
            .SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
    private string kitTransferStatus = "";
    private int kitTransferPercent;
    private TextView? kitTransferLabel;
    private ProgressBar? kitTransferProgress;
    private void UpdateKitTransfer(string status, int percent = 0)
    {
        kitTransferStatus = status; kitTransferPercent = percent;
        RunOnUiThread(() => {
            if (kitTransferLabel is not null) kitTransferLabel.Text = status;
            if (kitTransferProgress is not null) kitTransferProgress.Progress = percent;
        });
    }
    private void AddRestoreSlotButton(string path)
    {
        var b=new Button(this){Text="Restore this slot image",Enabled=connection.Snapshot.Device?.ProtocolVersion>=6&&!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.Snapshot.IsSimulated};
        b.Click+=(_,_)=>ConfirmSlot("Restore sample slot",
            "Stop the FM1 song. The current target slot is backed up before replacement. Choose the destination:",
            async(_,a)=> {
                try {
                    var bytes=File.ReadAllBytes(path);
                    await connection.EditDeviceAsync("Restoring sample slot…",async(c,t)=> {
                        var info=await c.RequestAsync(15,[],cancellationToken:t);
                        if(info.Arguments.Length<2||a.Which>=info.Arguments[0]||info.Arguments[1]!=80)throw new IOException("Unsupported destination slot.");
                        var current=await Fm1Operations.BackupSlot(c,(byte)a.Which,t);
                        var directory=System.IO.Path.Combine(FilesDir!.AbsolutePath,"slot-backups");
                        WorkspaceFiles.StoreBytes(System.IO.Path.Combine(directory,$"usr{a.Which+1}-restore-{Guid.NewGuid():N}.before.fm1"),current);
                        if(bytes.Length==0) {
                            var reply=await c.RequestAsync(14,[(byte)a.Which],timeout:TimeSpan.FromSeconds(10),cancellationToken:t);
                            if(reply.Arguments.Length!=2||reply.Arguments[0]!=a.Which||reply.Arguments[1]!=0|| (await Fm1Operations.BackupSlot(c,(byte)a.Which,t)).Length!=0)throw new IOException("Empty-slot restore could not be verified.");
                        } else await Fm1Operations.UploadSlot(c,(byte)a.Which,Sloop.Workstation.SlotImages.Read(bytes),_=>{},t);
                        return true;
                    });editing.SetStatus("Slot restored and verified.");
                }catch(Exception e){editing.SetStatus("Restore failed: "+e.Message);}
            });content.AddView(b);
    }
    private void AddKitUpload(SlotArtifact artifact)
    {
        kitTransferLabel=Label(kitTransferStatus,14);content.AddView(kitTransferLabel);
        kitTransferProgress=new ProgressBar(this,null,global::Android.Resource.Attribute.ProgressBarStyleHorizontal)
            {Max=100,Progress=kitTransferPercent,ContentDescription="Sample slot transfer progress"};content.AddView(kitTransferProgress);
        var button=new Button(this){Text="Send to FM1 · choose slot",Enabled=!samples.Busy&&!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.Snapshot.IsSimulated&&connection.Snapshot.Device?.ProtocolVersion>=6};
        button.Click+=(_,_)=>ConfirmProposedSampleSlot("Choose sample slot to replace",
            $"{artifact.Preview.Count} zones · {artifact.Data.Length:N0} bytes. Stop the FM1 song first. Choose a slot to back up and replace. If disconnected, restore its saved backup from Library.",
            async slot=> {
                try {
                    if(!ReferenceEquals(samples.Converted?.Artifact,artifact)) throw new IOException("The sample or mapping changed. Prepare the kit again before sending.");
                    if(kitDraftName!=samples.KitOptions.Name||!int.TryParse(kitDraftRoot,out var root)||root!=samples.KitOptions.RootNote)
                        throw new IOException("Kit name or root changed. Tap Prepare/Rebuild kit before sending.");
                    samples.StopPreview();
                    UpdateKitTransfer($"USR{slot+1} · backing up existing slot…");
                    await connection.EditDeviceAsync("Backing up slot and sending kit…",async(c,t)=> {
                        var info=await c.RequestAsync(15,[],cancellationToken:t);
                        if(info.Arguments.Length<2||slot>=info.Arguments[0]||info.Arguments[1]!=80)throw new IOException("Target slot is unsupported.");
                        var backup=await Fm1Operations.BackupSlot(c,(byte)slot,t);
                        var directory=System.IO.Path.Combine(FilesDir!.AbsolutePath,"slot-backups");Directory.CreateDirectory(directory);
                        var prefix=System.IO.Path.Combine(directory,$"usr{slot+1}-{DateTime.UtcNow:yyyyMMdd-HHmmss}-{Guid.NewGuid():N}");
                        WorkspaceFiles.StoreBytes(prefix+".before.fm1",backup);WorkspaceFiles.StoreBytes(prefix+".replacement.fm1",artifact.Image);
                        WorkspaceFiles.StoreBytes(prefix+".transfer",System.Text.Encoding.UTF8.GetBytes("Pending replacement. Restart from BEGIN; do not resume chunks after reconnect."));
                        UpdateKitTransfer($"USR{slot+1} · backup saved; sending…");
                        await Fm1Operations.UploadSlot(c,(byte)slot,artifact,bytes=> {
                            int percent=(int)(bytes*100L/Math.Max(1,artifact.Data.Length));
                            UpdateKitTransfer(percent==100?$"USR{slot+1} · verifying readback…":$"USR{slot+1} · sending {percent}%",percent);
                        },t);
                        WorkspaceFiles.StoreBytes(prefix+".transfer",System.Text.Encoding.UTF8.GetBytes("Acknowledged and verified by CRC-checked byte readback."));return true;
                    });
                    editing.SetStatus("Sample slot verified. Previous slot and replacement retained in phone backups.");
                    UpdateKitTransfer($"USR{slot+1} · sent and verified. Backup saved in Library.",100);
                    ShowWorkspace();
                    Toast.MakeText(this,"FM1 sample slot uploaded and verified.",ToastLength.Long)!.Show();
                }catch(Exception e){editing.SetStatus("Transfer interrupted: "+e.Message);UpdateKitTransfer("Send failed: "+e.Message+" Check Library for the saved backup.");ShowWorkspace();Toast.MakeText(this,e.Message,ToastLength.Long)!.Show();}
            });
        content.AddView(button);
    }
}
