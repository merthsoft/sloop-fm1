using Sloop.Workstation;
using Sloop.SampleEncoding;
namespace Sloop.Android.Services;
public sealed partial class SampleWorkspace
{
    private ConvertedKit? quickConverted;
    private string? quickConversionKey;
    public ConvertedKit? QuickConverted=>quickConverted is not null&&quickConversionKey==CurrentKey?quickConverted:null;
    public async Task<ConvertedKit?> PrepareQuickAsync(bool playableSound,string name)
    {
        if(Busy||Document is null)return null;
        if(!SetKitOptions(KitOptions with{Name=string.IsNullOrWhiteSpace(name)?playableSound?"SOUND":"CHOPS":name,
            Mapping=playableSound?SampleMapping.Instrument:SampleMapping.Chops,RootNote=playableSound?60:48,Loop=false}))return null;
        if(QuickConverted is {} ready)return ready;
        var doc=Document;var options=KitOptions;var key=CurrentKey;var edits=ChopOptions;
        Busy=true;StopPreview();conversion=new();var token=conversion.Token;
        Status="Getting your samples ready…";Notify();
        try {
            if(chopSettingsInvalid)throw new ArgumentException("Existing advanced chop settings are unreadable. Repair them in Advanced editor before sending.");
            var result=await Task.Run(()=>QuickSampling.Build(doc,options,playableSound,edits,token));
            token.ThrowIfCancellationRequested();WorkspaceFiles.StoreBytes(doc.Source.Path+".fm1",result.Artifact.Image);
            quickConverted=result;quickConversionKey=key;Status="Ready for FM1. Your original recording is kept.";return result;
        }catch(OperationCanceledException){Status="Preparation cancelled. Your original is kept.";return null;}
        catch(Exception e){Status=e.Message;return null;}
        finally{conversion.Dispose();conversion=null;Busy=false;Notify();}
    }
}
