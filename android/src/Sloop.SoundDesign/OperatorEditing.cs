namespace Sloop.SoundDesign;

public static class OperatorEditing
{
    public static Patch Copy(Patch patch, int source, int target)
    {
        PatchValidation.Require(patch);
        return patch.WithOperator(target, patch.GetOperator(source));
    }

    public static Patch Swap(Patch patch, int first, int second)
    {
        PatchValidation.Require(patch);
        var a = patch.GetOperator(first);
        var b = patch.GetOperator(second);
        return patch.WithOperator(first, b).WithOperator(second, a);
    }

    public static Patch SetEnvelopeLevel(Patch patch, int number, int stage, int level)
    {
        PatchValidation.Require(patch);
        if (level is < 0 or > 99) throw new ArgumentOutOfRangeException(nameof(level));
        var op = patch.GetOperator(number);
        var env = stage switch {
            1 => op.Envelope with { Level1 = level },
            2 => op.Envelope with { Level2 = level },
            3 => op.Envelope with { Level3 = level },
            4 => op.Envelope with { Level4 = level },
            _ => throw new ArgumentOutOfRangeException(nameof(stage))
        };
        return patch.WithOperator(number, op with { Envelope = env });
    }
}
