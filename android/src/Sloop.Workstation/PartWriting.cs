namespace Sloop.Workstation;

public static partial class PerformanceHarmony
{
    // At most 3^4 candidates: preserve every chord tone, with no voice crossing,
    // selected root pitch and bass register intact. Search only when voice count is stable.
    static int[] PartWritingVoicing(int[] chord,int root,int floor,int[] previous,int key,
        ChordVoicing? history,bool includeBass,int bass)
    {
        var candidates=new List<int[]>();
        var current=new int[chord.Length];
        void Search(int voice) {
            if(voice==chord.Length) {
                var c=current.Order().ToArray();
                if(c.Distinct().Count()==c.Length&&c.Contains(root)&&c[0]>=floor&&c[0]<floor+12&&c[^1]-c[0]<=24)candidates.Add(c);
                return;
            }
            for(int shift=-12;shift<=12;shift+=12) {
                int n=chord[voice]+shift;if(n is <0 or >127)continue;
                current[voice]=n;Search(voice+1);
            }
        }
        Search(0);
        if(candidates.Count==0)return chord;
        (int Parallels,int Tendencies,int OuterAndOverlap,int Leaps,int Motion,int Span) Score(int[] c) {
            if(c.SequenceEqual(previous))return (0,0,0,0,0,c[^1]-c[0]);
            var before=includeBass&&history is not null?new[]{history.Bass}.Concat(previous).ToArray():previous;
            var after=includeBass&&history is not null?new[]{bass}.Concat(c).ToArray():c;
            int parallels=0,tendencies=0,other=0,leaps=0,motion=0;
            for(int i=0;i<before.Length;i++) {
                int delta=after[i]-before[i];motion+=Math.Abs(delta);leaps+=Math.Max(0,Math.Abs(delta)-5);
                if(delta!=0) {
                    // Penalize missed resolution only if its target belongs to the new chord.
                    if(before[i]%12==(key+11)%12&&after.Any(n=>n%12==key)&&delta!=1)tendencies++;
                    if(history?.SeventhPitchClass==before[i]%12&&after.Any(n=>n%12==(before[i]+11)%12||n%12==(before[i]+10)%12)&&delta is not (-1 or -2))tendencies++;
                }
                for(int j=i+1;j<before.Length;j++) {
                    int a=Math.Abs(before[j]-before[i])%12,b=Math.Abs(after[j]-after[i])%12;
                    int d=after[j]-before[j];
                    if(delta!=0&&d!=0&&Math.Sign(delta)==Math.Sign(d)&&a==b&&b is 0 or 7)parallels++;
                }
                if(i+1<before.Length&&(after[i]>before[i+1]||after[i+1]<before[i]))other++;
            }
            int low=after[0]-before[0],high=after[^1]-before[^1],outer=Math.Abs(after[^1]-after[0])%12;
            if(low!=0&&Math.Sign(low)==Math.Sign(high)&&Math.Abs(high)>2&&outer is 0 or 7)other++;
            return (parallels,tendencies,other,leaps,motion,c[^1]-c[0]);
        }
        return candidates.OrderBy(Score).ThenBy(c=>c[0]).ThenBy(c=>c[1]).ThenBy(c=>c[2]).ThenBy(c=>c.Length==4?c[3]:0).First();
    }
}
