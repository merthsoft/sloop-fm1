using Sloop.Workstation;
int checks=0;
void Check(bool ok,string message){if(!ok)throw new Exception(message);checks++;}
int Parallels(int[] a,int[] b){
    int result=0;
    for(int i=0;i<a.Length;i++)for(int j=i+1;j<a.Length;j++) {
        int before=Math.Abs(a[j]-a[i])%12,after=Math.Abs(b[j]-b[i])%12;
        if(before==after&&before is 0 or 7&&(b[i]-a[i])*(b[j]-a[j])>0)result++;
    }
    return result;
}
var c=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,0,ChordShape.Diatonic,0,false);
var dSmooth=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,0,true,c.Notes);
var dParts=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,0,false,c.Notes,true,c);
Check(Parallels(c.Notes,dSmooth.Notes)==1&&Parallels(c.Notes,dParts.Notes)==0,"Open voicing avoids C-G to D-A parallel fifth while retaining all tones.");
Check(dParts.Notes.SequenceEqual(new[]{50,57,66}),"D major spreads A/F# into independent voices without moving the selected D.");
var bassParts=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,0,false,c.Notes,true,c,true);
Check(Parallels(new[]{c.Bass}.Concat(c.Notes).ToArray(),new[]{bassParts.Bass}.Concat(bassParts.Notes).ToArray())==1,"Optional bass participates: fixed root/bass octave parallel is unavoidable but extra fifth parallels disappear.");
var g=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,4,ChordShape.Diatonic,0,false);
var fParts=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,3,ChordShape.Diatonic,0,false,g.Notes,true,g);
Check(g.Notes[1]==59&&fParts.Notes[1]==60,"Leading tone B resolves up to C in the same voice when the target chord permits it.");
var minorFourth=PerformanceHarmony.Chord(0,PerformanceScale.NaturalMinor,3,3,ChordShape.Diatonic,0,false,g.Notes,true,g);
Check(minorFourth.Notes[1]==60,"A chromatically raised leading tone also resolves in natural minor.");
var g7=PerformanceHarmony.Chord(0,PerformanceScale.Mixolydian,3,4,ChordShape.Seventh,0,false);
Check(g7.SeventhPitchClass==5,"Chordal seventh metadata survives independently of voicing/inversion.");
var c7=PerformanceHarmony.Chord(0,PerformanceScale.Mixolydian,3,0,ChordShape.Seventh,0,false,g7.Notes,true,g7);
Check(g7.Notes[^1]==65&&c7.Notes[^1]==64,"Chordal seventh F resolves down to E in the same voice: "+string.Join(",",c7.Notes));
Check(PerformanceHarmony.Chord(0,PerformanceScale.Mixolydian,3,0,ChordShape.Seventh,0,false,c7.Notes,true,c7).Notes.SequenceEqual(c7.Notes),"Repeating a chord holds its chosen voicing.");
Check(PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,1,false,c.Notes,true,c).Notes.SequenceEqual(PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,1,false).Notes),"Explicit inversion remains authoritative.");
Check(PerformanceHarmony.Chord(0,PerformanceScale.Major,3,0,ChordShape.Seventh,0,false,c.Notes,true,c).Notes.SequenceEqual(PerformanceHarmony.Chord(0,PerformanceScale.Major,3,0,ChordShape.Seventh,0,true,c.Notes).Notes),"Changing voice count uses smooth fallback instead of inventing historical voices.");
Check(PerformanceHarmony.Chord(0,PerformanceScale.Major,3,1,ChordShape.Major,0,true,null,true).Notes.SequenceEqual(dSmooth.Notes),"No history seeds the original requested chord.");
for(int degree=8;degree<=21;degree++) {
    var high=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,degree,ChordShape.Diatonic,0,false,c.Notes,true,c);
    var equivalent=PerformanceHarmony.Chord(0,PerformanceScale.Major,3+degree/7,degree%7,ChordShape.Diatonic,0,false,c.Notes,true,c);
    Check(high.Notes.SequenceEqual(equivalent.Notes),"Extra pad octaves remain explicit in part-writing mode.");
}
foreach(int octave in new[]{-1,0,6,7})foreach(int degree in new[]{0,7,14,21}) {
    var edge=PerformanceHarmony.Chord(11,PerformanceScale.HarmonicMinor,octave,degree,ChordShape.Seventh,0,false,g7.Notes,true,g7);
    Check(edge.Notes.Length>0&&edge.Notes.All(n=>n is >=0 and <=127)&&edge.Notes.Distinct().Count()==edge.Notes.Length,"MIDI boundary voicings remain safe and unique.");
}
int transitions=0,improved=0;
foreach(var scale in Enum.GetValues<PerformanceScale>())for(int key=0;key<12;key++)
foreach(var shape in new[]{ChordShape.Diatonic,ChordShape.Seventh,ChordShape.Ninth,ChordShape.Sus2,ChordShape.Sus4})
for(int from=0;from<8;from++)for(int to=0;to<8;to++) {
    var previous=PerformanceHarmony.Chord(key,scale,3,from,shape,0,false);
    var smooth=PerformanceHarmony.Chord(key,scale,3,to,shape,0,true,previous.Notes);
    var parts=PerformanceHarmony.Chord(key,scale,3,to,shape,0,false,previous.Notes,true,previous);
    var plain=PerformanceHarmony.Chord(key,scale,3,to,shape,0,false);
    int before=Parallels(previous.Notes,smooth.Notes),after=Parallels(previous.Notes,parts.Notes);
    if(after>before)throw new Exception("Part writing introduced more parallel perfect intervals than smooth voicing.");
    if(after<before)improved++;
    if(!parts.Notes.Select(n=>n%12).Order().SequenceEqual(plain.Notes.Select(n=>n%12).Order()))throw new Exception("Part writing changed chord tones.");
    if(!parts.Notes.Contains(plain.Notes[0])||parts.Notes[0]<48+key+12*(to/7)||parts.Notes[0]>=60+key+12*(to/7))throw new Exception("Part writing ignored pad register.");
    if(!PerformanceHarmony.Chord(key,scale,3,to,shape,0,false,parts.Notes,true,parts).Notes.SequenceEqual(parts.Notes))throw new Exception("Part writing drifted on repeat.");
    var shiftedHistory=previous with{Notes=previous.Notes.Select(n=>n+12).ToArray(),Bass=previous.Bass+12};
    if(!PerformanceHarmony.Chord(key,scale,4,to,shape,0,false,shiftedHistory.Notes,true,shiftedHistory).Notes.SequenceEqual(parts.Notes.Select(n=>n+12)))throw new Exception("Part writing ignored octave transpose.");
    transitions++;
}
Check(transitions==26880&&improved>0,"All keys/scales/pads retain tone sets/register, repeats and octave choices, with no extra parallel perfect intervals.");
int chained=0;
foreach(var scale in Enum.GetValues<PerformanceScale>())for(int key=0;key<12;key++)
foreach(var shape in new[]{ChordShape.Diatonic,ChordShape.Seventh})foreach(bool useBass in new[]{false,true})
foreach(var progression in new[]{new[]{0,4,5,3},new[]{0,6,0,7,0},new[]{1,4,0,3,6}}) {
    var last=PerformanceHarmony.Chord(key,scale,3,0,shape,0,false);
    for(int repeat=0;repeat<4;repeat++)foreach(int degree in progression) {
        var next=PerformanceHarmony.Chord(key,scale,3,degree,shape,0,false,last.Notes,true,last,useBass);
        var smooth=PerformanceHarmony.Chord(key,scale,3,degree,shape,0,true,last.Notes);
        int[] Before(ChordVoicing v)=>useBass?new[]{v.Bass}.Concat(v.Notes).ToArray():v.Notes;
        if(Parallels(Before(last),Before(next))>Parallels(Before(last),Before(smooth)))throw new Exception("Chained part-writing progression introduced avoidable parallels.");
        var plain=PerformanceHarmony.Chord(key,scale,3,degree,shape,0,false);
        if(!next.Notes.Contains(plain.Notes[0]))throw new Exception("Chained progression drifted out of its selected register.");
        if(!PerformanceHarmony.Chord(key,scale,3,degree,shape,0,false,next.Notes,true,next,useBass).Notes.SequenceEqual(next.Notes))throw new Exception("Chained voicing changed on repeat.");
        last=next;chained++;
    }
}
Check(chained==18816,"Chained progressions with and without bass remain stable and avoid extra parallel perfect intervals.");
Console.WriteLine($"Part writing: {checks} checks; {transitions} transitions; fewer parallel perfect intervals in {improved} transitions; {chained} chained transitions.");
