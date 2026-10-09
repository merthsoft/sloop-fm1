using Android.Widget;
using Sloop.Workstation;
namespace Sloop.Android;
public sealed partial class MainActivity
{
    private void AddChopAudioEditor() {
        if(samples.Document is not {} doc) return;
        int index=selectedSampleSlice;var slice=doc.Slices()[index];
        var edit=ChopAudioSettings.Find(samples.ChopOptions,slice.Start,slice.End)??new ChopAudio(slice.Start,slice.End);
        Section("sample.chopaudio","Selected chop · pitch, gain & tuning",()=> {
            var gain=new EditText(this){Text=edit.Gain.ToString(System.Globalization.CultureInfo.InvariantCulture),Hint="Chop gain 0–8"};
            var tune=new EditText(this){Text=edit.TuneSemitones.ToString(System.Globalization.CultureInfo.InvariantCulture),Hint="Semitones −24…24 (changes duration)"};
            var root=new EditText(this){Text=edit.RootNote?.ToString()??"",Hint="MIDI root override 0–127; blank = mapping"};
            content.AddView(gain);content.AddView(tune);content.AddView(root);
            var detect=new Button(this){Text="Estimate pitch / root",Enabled=!samples.Busy};
            detect.Click+=async(_,_)=> {
                var estimate=await samples.EstimateChopAsync(index);if(estimate is null) return;
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Review root estimate")!
                    .SetMessage($"{SampleKitPlan.NoteName(estimate.RootNote)} · {estimate.Hertz:0.0} Hz · confidence {estimate.Confidence:P0}\nRaw source pitch; tuning remains separate. Applying changes this zone's MIDI root and mapping.")!
                    .SetPositiveButton("Apply root",(_,_)=>samples.SetChopAudio(edit with {RootNote=estimate.RootNote}))!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
            };content.AddView(detect);
            var save=new Button(this){Text="Save chop gain, tuning & root",Enabled=!samples.Busy};
            save.Click+=(_,_)=> {
                var culture=System.Globalization.CultureInfo.InvariantCulture;
                if(!double.TryParse(gain.Text,System.Globalization.NumberStyles.Float,culture,out var g)||!double.TryParse(tune.Text,System.Globalization.NumberStyles.Float,culture,out var t)||(!string.IsNullOrWhiteSpace(root.Text)&&!int.TryParse(root.Text,out _))) {Toast.MakeText(this,"Enter valid gain, tuning and MIDI root.",ToastLength.Long)!.Show();return;}
                samples.SetChopAudio(edit with {Gain=g,TuneSemitones=t,RootNote=string.IsNullOrWhiteSpace(root.Text)?null:int.Parse(root.Text!)});
            };content.AddView(save);
            var preview=new Button(this){Text="Listen with chop settings",Enabled=!samples.Busy};preview.Click+=async(_,_)=>await samples.PreviewChopAsync(index);content.AddView(preview);
        });
    }
}
