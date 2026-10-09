using System.Globalization;
using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
using Sloop.Workstation;

namespace Sloop.Android.Services;
public sealed partial class SampleWorkspace
{
    private KitSettings kitSettings=new("SLOOP",SampleMapping.Chops,60,MonoChoice.AverageChannels,1);
    private string? settingsSource;
    private ConvertedKit? converted;
    private string? conversionKey;
    private CancellationTokenSource? conversion;
    public byte[]? PendingKitExport {get;set;}
    public KitSettings KitOptions { get { LoadKitSettings(); return kitSettings; } }
    private string CurrentKey => Document is null ? "" : $"{Document.Source.Path}:{Document.Revision}:{Document.Start}:{Document.End}:{string.Join(',',Document.Markers)}:{KitOptions}:{ChopAudioSettings.Serialize(ChopOptions)}";
    public ConvertedKit? Converted => converted is not null && conversionKey==CurrentKey ? converted : null;
    private void LoadKitSettings()
    {
        if(Document is null || settingsSource==Document.Source.Path) return;
        settingsSource=Document.Source.Path; kitSettings=new("SLOOP",SampleMapping.Chops,60,MonoChoice.AverageChannels,1);
        try {
            if(!File.Exists(settingsSource+".kitsettings")) return;
            var l=File.ReadAllLines(settingsSource+".kitsettings");
            if(l.Length is not (6 or 7) || l[0]!="SLOOP-KIT-1") throw new FormatException("Unknown kit settings.");
            var restored=new KitSettings(System.Text.Encoding.UTF8.GetString(Convert.FromBase64String(l[1])),Enum.Parse<SampleMapping>(l[2]),int.Parse(l[3]),Enum.Parse<MonoChoice>(l[4]),double.Parse(l[5],CultureInfo.InvariantCulture),l.Length==7&&bool.Parse(l[6]));
            SampleKitPlan.Validate(restored, Document.Slices().Length);
            kitSettings=restored;
        } catch(Exception e) { Status="Could not restore conversion settings: "+e.Message; }
    }
    public bool SetKitOptions(KitSettings options)
    {
        if(Busy || Document is null) return false;
        try {
            LoadKitSettings();
            SampleKitPlan.Validate(options, Document.Slices().Length);
            if (options == kitSettings) return true;
            var lines=new[]{"SLOOP-KIT-1",Convert.ToBase64String(System.Text.Encoding.UTF8.GetBytes(options.Name)),options.Mapping.ToString(),options.RootNote.ToString(),options.Mono.ToString(),options.Gain.ToString("R",CultureInfo.InvariantCulture),options.Loop.ToString()};
            WorkspaceFiles.StoreBytes(settingsSource+".kitsettings",System.Text.Encoding.UTF8.GetBytes(string.Join('\n',lines)+"\n"));
            kitSettings=options; converted=null; Notify(); return true;
        } catch(Exception e) { Status="Could not save conversion settings: "+e.Message; Notify(); return false; }
    }
    public async Task ConvertKitAsync()
    {
        if(Busy || Document is null) return;
        var doc=Document; var options=KitOptions; var key=CurrentKey; var edits=ChopOptions;
        Busy=true; converted=null; StopPreview(); conversion=new(); var token=conversion.Token;
        Status="Converting chops to FM1 ADPCM…"; Notify();
        try {
            if(chopSettingsInvalid) throw new ArgumentException("Repair or explicitly save chop settings before conversion.");
            SampleKitPlan.Validate(options, doc.Slices().Length);
            var result=await Task.Run(()=>SamplePreparation.Build(doc,options,token,edits));
            token.ThrowIfCancellationRequested();
            WorkspaceFiles.StoreBytes(doc.Source.Path+".fm1",result.Artifact.Image);
            converted=result; conversionKey=key;
            Status=$"FM1 kit ready · {result.Artifact.Preview.Count} zones · {result.Artifact.Fit.DataBytes} / {SlotBuilder.Capacity} bytes\nClipped: {result.Reports.Sum(r=>r.ClippedSamples)} samples; channel cancellation: {result.Reports.Sum(r=>r.CancellationFrames)} frames. Original retained.";
        } catch(OperationCanceledException) { Status="Conversion cancelled. Original preserved."; }
        catch(Exception e) { Status="Conversion failed: "+e.Message; }
        finally { conversion.Dispose(); conversion=null; Busy=false; Notify(); }
    }
    public void CancelConversion() => conversion?.Cancel();
    public async Task PreviewConvertedAsync(int zone)
    {
        if(Busy || Converted is not { } kit || zone<0 || zone>=kit.Artifact.Preview.Count) return;
        var path=System.IO.Path.Combine(context.CacheDir!.AbsolutePath,"encoded-preview.wav");
        SamplePreparation.WritePreview(path,kit.Artifact.Preview[zone].DecodedPcm);
        var wave=PcmWave.Open(path); await PreviewAsync(0,wave.Frames,wave);
    }
    public Task PreviewConvertedChopAsync(int chop)
    {
        if (Converted is not {} kit) return Task.CompletedTask;
        // Wire zones sort by root, so a drum lane's source chop is not its preview index.
        for (int zone = 0; zone < kit.Artifact.Preview.Count; zone++)
            if (kit.Artifact.Preview[zone].InputIndex == chop) return PreviewConvertedAsync(zone);
        return Task.CompletedTask;
    }
    public async Task ExportKitAsync(global::Android.Net.Uri destination)
    {
        var image=PendingKitExport; PendingKitExport=null;
        if(image is null || Busy) return;
        Busy=true; Notify();
        try {
            await Task.Run(()=> { using var s=context.ContentResolver!.OpenOutputStream(destination,"wt")??throw new IOException("Cannot write kit."); s.Write(image); });
            Status="Exported FM1 slot image (.fm1). Original WAV retained.";
        } catch(Exception e) { Status="Kit export failed: "+e.Message; }
        finally { Busy=false; Notify(); }
    }
}



