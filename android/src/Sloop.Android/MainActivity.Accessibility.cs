using Android.Views;
using Android.Views.Accessibility;
using Android.Widget;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    readonly UiBoundedGesture accessiblePadGesture=new();
    readonly List<Button> accessiblePads=[];

    // Native buttons provide named focus/actions without changing the raw multitouch surface.
    void AddAccessiblePerformancePads(string[] labels)
    {
        accessiblePads.Clear();
        var manager=GetSystemService(AccessibilityService) as AccessibilityManager;
        Section("perform.accessible","Accessible pads · tap for a short note",()=> {
            content.AddView(Label("Each button plays a short note or chord and releases automatically, including with Latch enabled.",12));
            for(int row=0;row<(labels.Length+3)/4;row++) {
                var line=new LinearLayout(this){Orientation=Orientation.Horizontal};
                for(int col=0;col<4;col++) {
                    int cell=row*4+col;if(cell>=labels.Length)break;
                    var button=new Button(this){Text=labels[cell],ContentDescription=AccessiblePadName(cell,labels[cell])};
                    button.Click+=async(_,_)=> {
                        try {
                            if(!CanShowUi||state.Workspace!=Sloop.Core.Workspace.Perform)return;
                            int pointer=-100-cell;
                            await accessiblePadGesture.ActivateAsync(
                                ()=>PlayPerformance(pointer,cell,100,true,accessible:true),
                                ()=>connection.Performer.Release($"perform:{pointer}"),()=>Task.Delay(350));
                        }catch(Exception error){OnPerformanceFailure(error);}
                    };
                    accessiblePads.Add(button);
                    line.AddView(button,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
                }
                content.AddView(line);
            }
        },manager?.IsTouchExplorationEnabled==true);
    }
    static string AccessiblePadName(int cell,string label)=>$"Pad {cell+1}, {label.Replace('\n',' ')}. Play briefly, then release.";
    void UpdateAccessiblePadLabels()
    {
        var labels=PerformanceLabels();
        for(int i=0;i<Math.Min(labels.Length,accessiblePads.Count);i++) {
            accessiblePads[i].Text=labels[i];accessiblePads[i].ContentDescription=AccessiblePadName(i,labels[i]);
        }
    }
}
