using Android.Content;
using Android.Widget;
using Sloop.SampleEncoding;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private const int ExportKitRequest=413;
    private string? kitDraftSource;
    private string kitDraftName="SLOOP", kitDraftRoot="60";
    private void AddKitEditor()
    {
        if(samples.Document is not { } doc) return;
        content.AddView(Label("Map your chops",22));
        var options=samples.KitOptions;
        if(kitDraftSource!=doc.Source.Path) {kitDraftSource=doc.Source.Path;kitDraftName=options.Name;kitDraftRoot=options.RootNote.ToString();}
        var fit=SamplePreparation.Measure(doc,samples.ChopOptions);
        content.AddView(Label($"{fit.DataBytes:N0} / {fit.CapacityBytes:N0} ADPCM bytes · {(fit.Fits?"fits":"too large — trim before conversion")}",14));
        var capacity = new ProgressBar(this, null, global::Android.Resource.Attribute.ProgressBarStyleHorizontal)
            { Max = fit.CapacityBytes, Progress = (int)Math.Min(fit.DataBytes, fit.CapacityBytes), ContentDescription = "FM1 slot capacity used" };
        content.AddView(capacity);
        void Choice<T>(string title,T value,Action<T> set) where T:struct,Enum {
            var b=new Button(this){Text=$"{title} · {value}",Enabled=!samples.Busy};
            b.Click+=(_,_)=> new global::Android.App.AlertDialog.Builder(this)!.SetTitle(title)!
                .SetItems(Enum.GetNames<T>(),(_,a)=>set(Enum.GetValues<T>()[a.Which]))!.Show(); content.AddView(b);
        }
        Choice("Mapping",options.Mapping,v=>samples.SetKitOptions(samples.KitOptions with {Mapping=v}));
        Section("sample.encoding", "Audio settings · mono, gain & looping", () => {
        Choice("Mono conversion",options.Mono,v=>samples.SetKitOptions(samples.KitOptions with {Mono=v}));
        var loop=new CheckBox(this){Text="Loop each complete slice",Checked=options.Loop,Enabled=!samples.Busy};loop.SetTextColor(global::Android.Graphics.Color.White);
        loop.CheckedChange+=(_,a)=>samples.SetKitOptions(samples.KitOptions with{Loop=a.IsChecked});content.AddView(loop);
        var gain=new EditText(this){Text=options.Gain.ToString(System.Globalization.CultureInfo.InvariantCulture),Hint="Linear gain (0–8)",InputType=global::Android.Text.InputTypes.ClassNumber|global::Android.Text.InputTypes.NumberFlagDecimal}; content.AddView(gain);
        var saveGain=new Button(this){Text="Save gain",Enabled=!samples.Busy};
        saveGain.Click+=(_,_)=> {
            if(double.TryParse(gain.Text,System.Globalization.NumberStyles.Float,System.Globalization.CultureInfo.InvariantCulture,out var g)&&double.IsFinite(g)&&g is >=0 and <=8)
                samples.SetKitOptions(samples.KitOptions with {Gain=g});
            else Toast.MakeText(this,"Use gain 0–8.",ToastLength.Long)!.Show();
        };content.AddView(saveGain);
        });
        var name=new EditText(this){Text=kitDraftName,Hint="Kit name"};
        name.TextChanged+=(_,_)=>kitDraftName=name.Text??"";
        var root=new EditText(this){Text=kitDraftRoot,Hint="First MIDI root (0–127)",InputType=global::Android.Text.InputTypes.ClassNumber};
        root.TextChanged+=(_,_)=>kitDraftRoot=root.Text??"";
        Section("sample.identity", "Kit name & MIDI root", () => {
            content.AddView(Label("Kit name",14));content.AddView(name);
            content.AddView(Label("First MIDI root · 60 = middle C",14));content.AddView(root);
        });
        var build=new Button(this){Text=samples.Converted is null?"Prepare FM1 kit":"Rebuild kit",Enabled=!samples.Busy&&fit.Fits&&!connection.Snapshot.Busy};
        build.Click+=async(_,_)=> {
            if(!int.TryParse(root.Text,out var note)||note is <0 or >127) {
                Toast.MakeText(this,"Use root 0–127.",ToastLength.Long)!.Show(); return;
            }
            if(!samples.SetKitOptions(samples.KitOptions with{Name=name.Text??"SLOOP",RootNote=note})) return;
            await samples.ConvertKitAsync();
        }; content.AddView(build);
        if(samples.Busy) { var cancel=new Button(this){Text="Cancel conversion"}; cancel.Click+=(_,_)=>samples.CancelConversion(); content.AddView(cancel); }
        if(samples.Converted is not { } result) return;
        content.AddView(Label($"Ready · {result.Artifact.Preview.Count} mapped zones",18));
        AddKitUpload(result.Artifact);
        if(connection.Snapshot.Device is null||connection.Snapshot.IsSimulated)
            content.AddView(Label("Connect your FM1 using the top connection menu to send this kit. You can preview and export it offline.",14));
        var stop=new Button(this){Text="Stop preview"};stop.Click+=(_,_)=>samples.StopPreview();content.AddView(stop);
        Section("sample.export", "Export slot image", () => {
        var export=new Button(this){Text="Export FM1 slot image",Enabled=!samples.Busy}; export.Click+=(_,_)=> {
            samples.PendingKitExport=result.Artifact.Image.ToArray();
            var i=new Intent(Intent.ActionCreateDocument); i.AddCategory(Intent.CategoryOpenable); i.SetType("application/octet-stream"); i.PutExtra(Intent.ExtraTitle,"sloop-kit.fm1");
            StartActivityForResult(i,ExportKitRequest);
        }; content.AddView(export);
        });
        for(int index=0;index<result.Artifact.Preview.Count;index++) {
            int zone=index; var p=result.Artifact.Preview[index];
            var b=new Button(this){Text=$"▶ Encoded zone {index+1} · note {p.RootNote} · keys {p.LowNote}–{p.HighNote}",Enabled=!samples.Busy};
            b.Click+=async(_,_)=>await samples.PreviewConvertedAsync(zone); content.AddView(b);
        }
    }
}

