using Sloop.Workstation;
int checks=0;
void Check(bool value){checks++;if(!value)throw new Exception("Check "+checks);}
void Reject(Action action){bool rejected=false;try{action();}catch(ArgumentException){rejected=true;}Check(rejected);}
var map=new PerformanceMapping{Degrees=[6,5,4,3,2,1,0]};Check(map.Degree(0)==6);Check(map.Degree(7)==7);Reject(()=>new PerformanceMapping{Degrees=[7,1,2,3,4,5,6]}.Validate());
var x=new PerformanceMacroAxis(1,20,80,40);var y=new PerformanceMacroAxis(74,10,100,30,MacroCurve.Squared);
Reject(()=>new PerformanceMacroAxis(64).Validate());Reject(()=>new PerformanceMacroAxis(120).Validate());Reject(()=>new PerformanceMacroAxis(1,70,20).Validate());Reject(()=>new PerformanceMacroAxis(1,20,80,0).Validate());Reject(()=>x.Value(double.NaN));
Check(x.Value(-1)==20&&x.Value(2)==80);Check(y.Value(.5)==32);
var sent=new List<byte[]>();var player=new PerformanceMacroPlayer(b=>sent.Add(b));var macro=new PerformanceMacro(true,-1,x,y);
Check(player.Move(4,2,macro,.5,.5));Check(sent.Count==2&&sent[0].SequenceEqual(new byte[]{0xb2,1,50}));Check(!player.Move(5,2,macro,1,1));player.Move(4,2,macro,.5,.5);Check(sent.Count==2);
player.Release(5);Check(player.Active);player.Release(4);Check(!player.Active&&sent.Count==4&&sent[^2][2]==40&&sent[^1][2]==30);player.Stop();Check(sent.Count==4);
player.Move(6,2,macro,1,1);player.Forget();player.Stop();Check(sent.Count==6&&!player.Active);
Reject(()=>new PerformanceMacro(true,-1,x,x).Validate());
for(int i=0;i<=1000;i++){double p=i/1000.0;foreach(var curve in Enum.GetValues<MacroCurve>()){int n=(x with{Curve=curve}).Value(p);Check(n>=20&&n<=80);}}
var lower=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,map.Degree(6),ChordShape.Diatonic,0,false);var upper=PerformanceHarmony.Chord(0,PerformanceScale.Major,3,map.Degree(7),ChordShape.Diatonic,0,false);Check(upper.Notes[0]-lower.Notes[0]==12);
var failures=new List<int>();var failing=new PerformanceMacroPlayer(b=>{failures.Add(b[1]);if(b[2]==40)throw new IOException("reset failed");});failing.Move(1,0,macro,1,1);try{failing.Stop();}catch(IOException){}Check(!failing.Active&&failures.Count==4&&failures[^1]==74);
Console.WriteLine($"Performance controls: {checks} checks passed.");
