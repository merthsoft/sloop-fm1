namespace Sloop.Workstation;

/// <summary>One momentary CC1 owner; release and cancellation restore zero.</summary>
public sealed class ModulationPlayer(Action<byte[]> send)
{
    int? owner; int channel, last=-1;
    public bool Active=>owner.HasValue;
    public bool Move(int pointer,int destination,int amount) {
        if(destination is <0 or >15 || amount is <0 or >127)throw new ArgumentOutOfRangeException();
        if(owner.HasValue && owner!=pointer)return false;
        if(owner.HasValue && channel!=destination)throw new InvalidOperationException("Release modulation before changing channel.");
        owner=pointer;channel=destination;
        if(last!=amount){send([(byte)(0xb0+channel),1,(byte)amount]);last=amount;}
        return true;
    }
    public void Release(int pointer){if(owner==pointer)Stop();}
    public void Stop(){bool active=owner.HasValue;owner=null;last=-1;if(active)send([(byte)(0xb0+channel),1,0]);}
    public void Forget(){owner=null;last=-1;}
}
