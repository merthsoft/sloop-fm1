using System.Globalization;
using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
using Sloop.Workstation;

namespace Sloop.Android.Services;
public sealed partial class SampleWorkspace
{
    public sealed record Prepared(string Directory,SampleDocument? Document,float[] Peaks,KitSettings Options,long Cursor);
    public Prepared Capture()=>new(directory,Document,Peaks,KitOptions,Cursor);
    public static Task<Prepared> PrepareAsync(string root,bool includePeaks=true)=>Task.Run(()=>
    {
        var directory=System.IO.Path.Combine(root,"samples");Directory.CreateDirectory(directory);
        SampleDocument? document=null;float[] peaks=[];KitSettings options=new("SLOOP",SampleMapping.Chops,60,MonoChoice.AverageChannels,1);
        var pointer=System.IO.Path.Combine(directory,"current.txt");
        if(File.Exists(pointer))
        {
            var name=File.ReadAllText(pointer);SessionStore.RequirePath(name);if(name.Contains('/'))throw new InvalidDataException("Invalid sample pointer.");
            var source=PcmWave.Open(System.IO.Path.Combine(directory,name));
            ChopAudioSettings.ValidateFile(source);
            document=File.Exists(source.Path+".edits")?SampleDocument.Restore(source,source.Path+".edits"):new(source);if(includePeaks)peaks=source.Peaks();
            if(File.Exists(source.Path+".kitsettings"))
            {
                var l=File.ReadAllLines(source.Path+".kitsettings");if(l.Length is not (6 or 7)||l[0]!="SLOOP-KIT-1")throw new InvalidDataException("Invalid kit settings.");
                options=new(System.Text.Encoding.UTF8.GetString(Convert.FromBase64String(l[1])),Enum.Parse<SampleMapping>(l[2]),int.Parse(l[3],CultureInfo.InvariantCulture),Enum.Parse<MonoChoice>(l[4]),double.Parse(l[5],CultureInfo.InvariantCulture),l.Length==7&&bool.Parse(l[6]));
                if(!Enum.IsDefined(options.Mapping)||!Enum.IsDefined(options.Mono)||options.RootNote is <0 or >127||!double.IsFinite(options.Gain)||options.Gain is <0 or >8||options.Name.Length>80)throw new InvalidDataException("Invalid kit setting values.");
            }
        }
        return new Prepared(directory,document,peaks,options,0);
    });
    public void Adopt(Prepared prepared)
    {
        if(Busy)throw new InvalidOperationException("Wait for sample work before adopting a workspace.");
        StopPreview();directory=prepared.Directory;Document=prepared.Document;Peaks=prepared.Peaks;Cursor=prepared.Cursor;
        kitSettings=prepared.Options;settingsSource=Document?.Source.Path;converted=null;conversionKey=null;PendingExport=null;PendingKitExport=null;
        Status="Session sample loaded; original retained.";
    }
}
