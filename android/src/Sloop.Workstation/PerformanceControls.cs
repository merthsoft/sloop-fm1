namespace Sloop.Workstation;

/// <summary>Serializable mapping; upper tonic is deliberately not remappable.</summary>
public sealed record PerformanceMapping
{
    public int[] Degrees { get; init; } = [0,1,2,3,4,5,6];
    public ChordShape[] Directions { get; init; } = [ChordShape.Seventh,ChordShape.Ninth,ChordShape.Sus4,ChordShape.Sus2,ChordShape.Minor,ChordShape.Major,ChordShape.InversionDown,ChordShape.InversionUp];
    public void Validate() {
        if(Degrees is null || Degrees.Length!=7 || Degrees.Any(d=>d is <0 or >6) || Directions is null || Directions.Length!=8 || Directions.Any(d=>!Enum.IsDefined(d)))
            throw new ArgumentException("Mappings require seven lower degrees (0–6) and eight valid directions.");
    }
    public int Degree(int cell) { Validate();return cell==7?7:cell is >=0 and <7?Degrees[cell]:throw new ArgumentOutOfRangeException(nameof(cell)); }
}

public enum MacroCurve { Linear, Squared, SquareRoot }
public sealed record PerformanceMacroAxis(int Controller=1,int Minimum=0,int Maximum=127,int Default=0,MacroCurve Curve=MacroCurve.Linear)
{
    public void Validate() {
        // Exclude sustain, mode messages and RPN/NRPN/data-entry commands: these need dedicated ownership.
        if(Controller is <0 or >119 || Controller is 6 or 38 or 64 or 96 or 97 or 98 or 99 or 100 or 101 || Minimum is <0 or >127 || Maximum<Minimum || Maximum>127 || Default<Minimum || Default>Maximum || !Enum.IsDefined(Curve))
            throw new ArgumentException("Invalid or ownership-sensitive macro CC/range/default.");
    }
    public int Value(double position) {
        Validate();if(!double.IsFinite(position))throw new ArgumentOutOfRangeException(nameof(position));
        double p=Math.Clamp(position,0,1);p=Curve switch {MacroCurve.Squared=>p*p,MacroCurve.SquareRoot=>Math.Sqrt(p),_=>p};
        return Math.Clamp((int)Math.Round(Minimum+(Maximum-Minimum)*p),Minimum,Maximum);
    }
}
public sealed record PerformanceMacro(bool Enabled=false,int Track=-1,PerformanceMacroAxis? X=null,PerformanceMacroAxis? Y=null)
{
    public PerformanceMacroAxis XAxis=>X??new(1);
    public PerformanceMacroAxis YAxis=>Y??new(74);
    public void Validate(){if(Track is <-1 or >2)throw new ArgumentException("Choose selected track or synth 1–3.");XAxis.Validate();YAxis.Validate();if(XAxis.Controller==YAxis.Controller)throw new ArgumentException("XY axes must have different CCs.");}
}

/// <summary>Single pointer ownership. Quantized duplicate values are coalesced. Release restores declared defaults.</summary>
public sealed class PerformanceMacroPlayer(Action<byte[]> send)
{
    int? owner;int channel;PerformanceMacro? macro;readonly Dictionary<int,int> last=[];
    public bool Active=>owner.HasValue;
    public bool Move(int pointer,int destination,PerformanceMacro settings,double x,double y) {
        settings.Validate();if(destination is <0 or >15)throw new ArgumentOutOfRangeException(nameof(destination));
        if(!settings.Enabled)return false;
        if(owner.HasValue&&owner!=pointer)return false;
        if(owner.HasValue&&(channel!=destination||macro!=settings))throw new InvalidOperationException("Release the macro before changing its destination.");
        owner=pointer;channel=destination;macro=settings;
        Emit(settings.XAxis.Controller,settings.XAxis.Value(x));Emit(settings.YAxis.Controller,settings.YAxis.Value(y));return true;
    }
    void Emit(int cc,int value){if(last.TryGetValue(cc,out var old)&&old==value)return;send([(byte)(0xb0+channel),(byte)cc,(byte)value]);last[cc]=value;}
    public void Release(int pointer){if(owner==pointer)Stop();}
    public void Stop(){var saved=macro;owner=null;macro=null;try{if(saved is not null){try{Emit(saved.XAxis.Controller,saved.XAxis.Default);}finally{Emit(saved.YAxis.Controller,saved.YAxis.Default);}}}finally{last.Clear();}}
    public void Forget(){owner=null;macro=null;last.Clear();}
}
