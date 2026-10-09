using Sloop.Workstation;
namespace Sloop.Android;

public enum PerformSurface { Chords, ScaleGrid, Keyboard, DrumPads, Ribbon }
public sealed record PerformOptions(PerformSurface Surface=PerformSurface.Chords,int Key=0,PerformanceScale Scale=PerformanceScale.Major,
    int Octave=3,int Velocity=100,int Tempo=100,int Division=4,int StrumMs=35,int Inversion=0,bool VoiceLead=true,bool Latch=false,int BassTrack=-1,PlayStyle Style=PlayStyle.Block,int ChannelOverride=-1,bool FollowHardwareOctave=true,
    PerformanceMapping? Mapping=null,PerformanceMacro? Macro=null);
