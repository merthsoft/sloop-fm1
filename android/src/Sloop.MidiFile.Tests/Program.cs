using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.Workstation;

int checks=0;
void Check(bool condition,string message){if(!condition)throw new Exception(message);checks++;}
void Reject(byte[] bytes){try{MidiFile.Read(bytes);}catch(InvalidDataException){checks++;return;}throw new Exception("Malformed MIDI accepted");}
byte[] Chunk(string tag,byte[] bytes)=>[..System.Text.Encoding.ASCII.GetBytes(tag),(byte)(bytes.Length>>24),(byte)(bytes.Length>>16),(byte)(bytes.Length>>8),(byte)bytes.Length,..bytes];
byte[] File(int format,params byte[][] tracks)=>[..Chunk("MThd",[0,(byte)format,0,(byte)tracks.Length,1,224]),..tracks.SelectMany(t=>Chunk("MTrk",t))];
var bytes=File(1,[0,255,81,3,7,161,32,0,255,47,0],
    [0,255,3,4,76,101,97,100,0,144,60,100,0,60,80,0,145,64,90,129,112,128,60,0,0,60,0,0,129,64,0,0,255,47,0]);
var file=MidiFile.Read(bytes);
Check(file.Ppq==480&&file.Tempo==120&&file.Parts.Length==2,"format 1 conductor, channels and tempo");
Check(file.Parts[0].Name=="Lead"&&file.Parts[0].Notes.Length==2&&file.Parts[0].Notes.All(n=>n.Duration==240),"running status and overlapping same pitch release FIFO");
Check(file.Parts[1].Channel==1&&file.Parts[1].Notes[0].Pitch==64,"separate parts per source channel");
for(int i=0;i<bytes.Length;i++)Reject(bytes[..i]);
Reject(File(2,[0,255,47,0]));
Reject(File(0,[0,60,100,0,255,47,0]));
Reject(File(0,[128,128,128,128,0,144,60,100]));
Reject(File(0,[0,144,60,255]));
Reject(File(0,[0,255,81,3,0,0,0]));
var smpte=bytes.ToArray();smpte[12]=231;Reject(smpte);
var skipped=MidiFile.Read(File(0,[0,176,64,127,0,240,2,1,2,0,144,60,100,131,96,255,47,0]));
Check(skipped.Warnings.Length==3&&skipped.Parts[0].Notes[0].Duration==480,"explicit sustain/SysEx/missing-off warnings");
Reject(File(0,[0,240,1,0,0,60,100]));
Reject(File(0,[0,144,60,100,0,255,1,0,0,60,0]));
using(var stream=new MemoryStream(bytes))Check(MidiFile.Read(stream).Parts.Length==2,"stream import");
Reject(new byte[MidiFile.MaxBytes+1]);
var zeroOff=MidiFile.Read(File(0,[0,144,60,100,129,112,60,0,0,255,47,0]));
Check(zeroOff.Parts[0].Notes[0].Duration==240,"velocity zero releases with running status");
var changedTempo=MidiFile.Read(File(0,[0,255,81,3,7,161,32,0,144,60,100,129,112,255,81,3,15,66,64,0,128,60,0,0,255,47,0]));
Check(changedTempo.Warnings.Any(w=>w.Contains("Tempo changes")),"tempo map warning");
var source=new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(3840),[]);
var keep=new AppNote(Guid.NewGuid(),source.Id,new(0),new(100),72,99,1);
source=source with{Notes=[keep,new(Guid.NewGuid(),source.Id,new(0),new(100),50,90,0)]};
var after=MidiFile.ToApp(file,file.Parts[0],source,0);
Check(after.Notes.Length==3&&after.Notes.Contains(keep)&&after.Notes.Where(n=>n.Channel==0).All(n=>n.Duration.Value==480),"destination replaced, others preserved, PPQ rescaled");
var history=new EditHistory(source);history.Apply(EditProposal.Between(source,after,new("MIDI","test")));history.Undo();
Check(((AppPattern)history.Current).Notes.SequenceEqual(source.Notes),"import undo restores original notes");
HardwareTrack Track(bool drum)=>new(Guid.NewGuid(),drum,16,Enumerable.Range(0,64).Select(_=>HardwareEditor.Blank(drum)).ToImmutableArray(),[],new Dictionary<int,int>{{29,16},{30,2}}.ToImmutableDictionary(),new("test",[],ImmutableDictionary<int,ParameterRange>.Empty));
var native=new HardwarePattern(Guid.NewGuid(),Guid.NewGuid(),[Track(false),Track(false),Track(false),Track(true)]);
var part=new MidiFilePart("Chords",0,[new(0,480,60,80),new(0,480,64,90),new(0,480,67,100),new(0,480,71,110),new(0,480,74,120),new(0,480,74,90),new(240,120,62,80),new(2400,120,60,90)]);
var plan=MidiFileHardware.Convert(file,part,native,1);
var chord=(SynthStep)plan.Pattern.Tracks[1].Steps[0];
Check(chord.Count==4&&chord.Slots.Select(n=>n.Pitch).SequenceEqual(new[]{64,67,71,74})&&chord.Velocity==120,"strongest four unique notes in ascending pitch order");
Check(((SynthStep)plan.Pattern.Tracks[1].Steps[1]).Time==StepTime.Tie&&((SynthStep)plan.Pattern.Tracks[1].Steps[2]).Time==StepTime.Note,"ties stop at next onset");
Check(plan.Report.Contains("1 notes beyond active length")&&plan.Report.Contains("1 duplicates merged")&&plan.Report.Contains("1 notes beyond four")&&plan.Report.Contains("1 chords shortened"),"conversion loss report");
Check(ReferenceEquals(plan.Pattern.Tracks[0],native.Tracks[0])&&ReferenceEquals(plan.Pattern.Tracks[1].Steps[16],native.Tracks[1].Steps[16])&&plan.Pattern.Tracks[1].Parameters==native.Tracks[1].Parameters,"other tracks, dormant steps and settings unchanged");
Check(plan.Preview.Notes.Length==5&&plan.Preview.Notes[0].Duration.Value==480,"audition reflects converted chord duration and capacity");
var drums=MidiFileHardware.Convert(file,new("Drums",9,[new(0,20,35,50),new(0,20,36,100),new(0,20,42,127),new(0,20,44,70)]),native,3);
Check(((DrumStep)drums.Pattern.Tracks[3].Steps[0]).Lanes.Count(n=>n.On)==4,"distinct GM drum lanes");
Check(drums.Preview.Notes.Select(n=>n.Pitch).Order().SequenceEqual(new[]{35,36,42,44}),"converted drum audition uses canonical firmware pitches");
Check(drums.Preview.Notes.All(n=>n.Channel==9),"GM drum preview channel");
foreach(int div in Enumerable.Range(0,9)) {
    var tr=native.Tracks[0] with{Parameters=native.Tracks[0].Parameters.SetItem(30,div)};
    var converted=MidiFileHardware.Convert(file,zeroOff.Parts[0],native with{Tracks=native.Tracks.SetItem(0,tr)},0);
    PatternValidation.Validate(converted.Pattern);PatternValidation.Validate(converted.Preview);checks++;
    var lowPpq=MidiFileHardware.Convert(file with{Ppq=1},new("Low PPQ",0,[new(1,1,60,100)]),native with{Tracks=native.Tracks.SetItem(0,tr)},0);
    Check(lowPpq.Preview.TicksPerQuarter==960,"native audition uses precise timing even with low source PPQ");
}
// Every truncation must fail cleanly, never leak an index error or loop forever.
var random=new Random(7419);
for(int i=0;i<1000;i++) {var fuzz=bytes.ToArray();fuzz[random.Next(fuzz.Length)]=(byte)random.Next(256);try{MidiFile.Read(fuzz);}catch(InvalidDataException){} checks++;}
Console.WriteLine($"MIDI import: {checks} checks passed.");
