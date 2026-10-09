using System.Text.RegularExpressions;
using Sloop.SoundDesign;

int checks=0;
void Check(bool condition,string message) { checks++; if(!condition)throw new Exception(message); }
void Reject(Action action) { try {action();} catch(ArgumentException) {checks++;return;} throw new Exception("Invalid edit accepted"); }

// Compare against the firmware's own table, not a second copy of the app table.
string firmware=File.ReadAllText(Path.Combine(AppContext.BaseDirectory,"fm6_core.c"));
string table=Regex.Match(firmware,@"FM6_ALG\[32\]\[6\]\s*=\s*\{([\s\S]*?)\};").Groups[1].Value;
var rows=Regex.Matches(table,@"\{([^{}]+)\}").Select(m=>Regex.Matches(m.Groups[1].Value,@"0x[0-9a-fA-F]+")
    .Select(v=>Convert.ToInt32(v.Value,16)).ToArray()).ToArray();
Check(rows.Length==32&&rows.All(r=>r.Length==6),"Firmware table parsed");
for(int algorithm=1;algorithm<=32;algorithm++) {
    var graph=FirmwareAlgorithms.Get(algorithm);
    var buses=new HashSet<int>[] {[],[],[]};
    var expected=new HashSet<ModulationEdge>();
    var feedback=new List<int>();
    for(int index=0;index<6;index++) {
        int op=6-index, flags=rows[algorithm-1][index], input=(flags>>4)&3, output=flags&3;
        Check(graph.Carriers.Contains(op)==(output==0),$"Algorithm {algorithm} OP{op} carrier");
        Check(AlgorithmTopology.IsCarrier(algorithm,op)==graph.Carriers.Contains(op),"Existing carrier mask agrees");
        if(input!=0)foreach(int modulator in buses[input])expected.Add(new(modulator,op));
        if((input==0||buses[input].Count==0)&&(flags&0xc0)==0xc0)feedback.Add(op);
        if((flags&4)==0)buses[output].Clear();
        buses[output].Add(op);
    }
    Check(expected.SetEquals(graph.Edges),$"Algorithm {algorithm} modulation buses");
    Check(feedback.SequenceEqual(graph.FeedbackOperators),$"Algorithm {algorithm} renderer feedback");
    Check(graph.Edges.All(e=>graph.Depth(e.Source)>graph.Depth(e.Target)),"Layout follows modulation");
}
Check(FirmwareAlgorithms.Get(1).Edges.Contains(new(6,5)),"OP6 feeds OP5");
Check(FirmwareAlgorithms.Get(4).FeedbackOperators.SequenceEqual([6]),"Renderer uses OP6 self feedback for algorithm 4");
Check(FirmwareAlgorithms.Get(32).Edges.IsEmpty&&FirmwareAlgorithms.Get(32).Carriers.Length==6,"Algorithm 32 is additive");
Reject(()=>FirmwareAlgorithms.Get(0));Reject(()=>FirmwareAlgorithms.Get(33));

var original=FactoryLibrary.Get(0);
var patch=original.WithOperator(6,original.GetOperator(6) with {OutputLevel=41,Fine=73});
for(int source=1;source<=6;source++)for(int target=1;target<=6;target++) {
    var copy=OperatorEditing.Copy(patch,source,target);
    var swap=OperatorEditing.Swap(patch,source,target);
    for(int number=1;number<=6;number++) {
        Check(copy.GetOperator(number)==patch.GetOperator(number==target?source:number),"Copy changes only destination");
        Check(swap.GetOperator(number)==patch.GetOperator(number==source?target:number==target?source:number),"Swap preserves other operators");
    }
    Check(PatchCodec.ContentEquals(OperatorEditing.Swap(swap,source,target),patch),"Swap twice restores full patch");
    Check(copy with {Operators=patch.Operators}==patch,"Copy preserves globals");
    Check(swap with {Operators=patch.Operators}==patch,"Swap preserves globals");
}
for(int stage=1;stage<=4;stage++)foreach(int level in new[]{0,47,99}) {
    var edited=OperatorEditing.SetEnvelopeLevel(patch,6,stage,level);
    byte[] before=PatchCodec.EncodeVoice(patch),after=PatchCodec.EncodeVoice(edited);
    for(int i=0;i<before.Length;i++)Check(after[i]==(i==stage+3?level:before[i]),"Envelope edits only selected OP6 level");
}
Check(patch.GetOperator(6).Envelope==original.GetOperator(6).Envelope,"Original envelope remains immutable");
Reject(()=>OperatorEditing.Copy(patch,0,1));Reject(()=>OperatorEditing.Swap(patch,1,7));
Reject(()=>OperatorEditing.SetEnvelopeLevel(patch,1,0,5));Reject(()=>OperatorEditing.SetEnvelopeLevel(patch,1,1,100));
Console.WriteLine($"FM6 editing: {checks} checks passed.");
