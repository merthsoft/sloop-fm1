using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Scenes;
public enum AutomationShape { Hold, Linear }
public enum AutomationParameter { MidiControlChange, SoundMacro }
public sealed record AutomationTarget(string TargetId, AutomationParameter Parameter, int Index, int Minimum, int Maximum);
public sealed record AutomationPoint(Tick Position, int Value);
public sealed record AutomationLane(Guid Id, AutomationTarget Target, AutomationShape Shape, ImmutableArray<AutomationPoint> Points)
{
    public void Validate(Tick length)
    {
        if (Id == Guid.Empty || string.IsNullOrWhiteSpace(Target.TargetId) || !Enum.IsDefined(Shape) ||
            !Enum.IsDefined(Target.Parameter) || Target.Minimum > Target.Maximum || Points.IsDefaultOrEmpty || length.Value <= 0 ||
            (Target.Parameter == AutomationParameter.MidiControlChange && (Target.Index is < 0 or > 127 || Target.Minimum < 0 || Target.Maximum > 127)) ||
            (Target.Parameter == AutomationParameter.SoundMacro && Target.Index is < 0 or > 6))
            throw new ArgumentException("Invalid automation lane or target.");
        if (Target.Parameter == AutomationParameter.SoundMacro)
        {
            var range = Target.Index switch { 0 => (0,32), 1 => (-7,7), 2 => (-64,63), 3 => (-16,16),
                4 => (-64,63), 5 => (-7,7), 6 => (0,127), _ => throw new ArgumentException("Unknown macro.") };
            if (Target.Minimum < range.Item1 || Target.Maximum > range.Item2)
                throw new ArgumentException("Automation exceeds existing MacroContext ranges.");
        }
        long previous = -1;
        foreach (var point in Points)
        {
            if (point.Position.Value <= previous || point.Position.Value >= length.Value || point.Value < Target.Minimum || point.Value > Target.Maximum)
                throw new ArgumentException("Automation points must be ordered, unique and in range.");
            previous = point.Position.Value;
        }
    }
    public int? Evaluate(Tick position, Tick length)
    {
        Validate(length);
        if (position.Value < 0 || position.Value >= length.Value) throw new ArgumentOutOfRangeException(nameof(position));
        if (position.Value < Points[0].Position.Value) return null;
        for (int i = 1; i < Points.Length; i++)
            if (position.Value < Points[i].Position.Value)
            {
                var a = Points[i - 1]; var b = Points[i];
                return Shape == AutomationShape.Hold ? a.Value : (int)Math.Round(a.Value +
                    ((double)b.Value - a.Value) * (position.Value - a.Position.Value) / (b.Position.Value - a.Position.Value), MidpointRounding.AwayFromZero);
            }
        return Points[^1].Value;
    }
}
