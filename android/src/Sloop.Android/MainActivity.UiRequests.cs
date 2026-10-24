using Android.App;
using Android.Content;
using Android.OS;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    Guid? soundImportRequest, fileExportRequest;
    readonly UiRequestLifetime sampleUiRequest=new(), compositionUiRequest=new();
    bool activityStopped,activityDestroyed;
    bool CanShowUi=>!activityStopped&&!activityDestroyed&&!IsFinishing&&!IsDestroyed;
    UiFileRequests FileRequests=>new(System.IO.Path.Combine(FilesDir!.AbsolutePath,"ui-requests"));

    void RestoreUiRequests(Bundle? saved)
    {
        if(Guid.TryParse(saved?.GetString("sound-import-request"),out var import))soundImportRequest=import;
        if(Guid.TryParse(saved?.GetString("file-export-request"),out var export))fileExportRequest=export;
        try {FileRequests.RemoveExpired(DateTime.UtcNow.AddDays(-7),soundImportRequest,fileExportRequest);}
        catch(IOException error){global::Android.Util.Log.Warn("MerthSloop","Picker cleanup deferred: "+error.Message);}
    }
    void SaveUiRequests(Bundle saved)
    {
        saved.PutString("sound-import-request",soundImportRequest?.ToString());
        saved.PutString("file-export-request",fileExportRequest?.ToString());
    }
    void LaunchFileExport()
    {
        if(fileExportRequest is not {} id)throw new IOException("Choose Export again.");
        var request=FileRequests.RequireExport(id);
        var intent=new Intent(Intent.ActionCreateDocument);intent.AddCategory(Intent.CategoryOpenable);
        intent.SetType(request.Mime);intent.PutExtra(Intent.ExtraTitle,request.Name);
        StartActivityForResult(intent,ExportFileRequest);
    }
    void StageFileExport(string name,string mime,Action<Stream> write)
    {
        if(fileExportRequest is {} old)FileRequests.Complete(old);
        fileExportRequest=FileRequests.StageExport(name,mime,write).Id;
        try {LaunchFileExport();}
        catch {FileRequests.Complete(fileExportRequest.Value);fileExportRequest=null;throw;}
    }
    void FinishFileExport(Guid id)
    {
        FileRequests.Complete(id);if(fileExportRequest==id)fileExportRequest=null;
    }
    void OfferExportRetry(Guid id)
    {
        if(!CanShowUi||fileExportRequest!=id)return;
        new AlertDialog.Builder(this)!.SetTitle("Export failed")!
            .SetMessage("The prepared file is retained. Choose another destination to retry.")!
            .SetPositiveButton("Retry",(_,_)=> {
                try {LaunchFileExport();}catch(Exception error){editing.SetStatus("Export retry failed: "+error.Message);}
            })!.SetNegativeButton("Discard",(_,_)=>editing.Run(()=>FinishFileExport(id),"Prepared export discarded."))!.Show();
    }
    bool CurrentSampleRequest(long request,object source,string root)=>sampleUiRequest.IsCurrent(
        request,CanShowUi,source,samples.Document,root,EffectiveWorkspaceRoot);
    bool CurrentCompositionRequest(long request,object source,string root)=>compositionUiRequest.IsCurrent(
        request,CanShowUi,source,editing.Sequence.Current,root,EffectiveWorkspaceRoot);
}
