using System.Collections.Immutable;

namespace Sloop.SoundDesign;

public enum FrequencyMode { Ratio, Fixed }
public enum ScalingCurve { NegativeLinear, NegativeExponential, PositiveExponential, PositiveLinear }
public enum LfoWave { Triangle, SawDown, SawUp, Square, Sine, SampleAndHold }
public sealed record Envelope(int Rate1, int Rate2, int Rate3, int Rate4,
    int Level1, int Level2, int Level3, int Level4);
public sealed record Operator(Envelope Envelope, int Breakpoint = 39, int LeftDepth = 0,
    int RightDepth = 0, ScalingCurve LeftCurve = 0, ScalingCurve RightCurve = 0,
    int RateScaling = 0, int AmplitudeSensitivity = 0, int VelocitySensitivity = 0,
    int OutputLevel = 0, FrequencyMode Mode = 0, int Coarse = 1, int Fine = 0, int Detune = 7);

/// <summary>Operators in musical order OP1..OP6; algorithm 1..32. Transpose 24 is neutral.</summary>
public sealed record Patch(ImmutableArray<Operator> Operators, Envelope PitchEnvelope,
    int Algorithm, int Feedback, bool OscillatorSync, int LfoSpeed, int LfoDelay,
    int LfoPitchDepth, int LfoAmplitudeDepth, bool LfoSync, LfoWave LfoWave,
    int PitchSensitivity, int Transpose, string Name)
{
    public Operator GetOperator(int number) => number is >= 1 and <= 6
        ? Operators[number - 1] : throw new ArgumentOutOfRangeException(nameof(number));
    public Patch WithOperator(int number, Operator value) => this with
    { Operators = Operators.SetItem(number is >= 1 and <= 6 ? number - 1
        : throw new ArgumentOutOfRangeException(nameof(number)), value) };
}

public static class PatchValidation
{
    internal static readonly int[] OperatorMax = [99,99,99,99,99,99,99,99,99,99,99,3,3,7,3,7,99,1,31,99,14];
    internal static readonly int[] VoiceMax = [99,99,99,99,99,99,99,99,31,7,1,99,99,99,99,1,5,7,48];
    public static ImmutableArray<string> Errors(Patch patch)
    {
        var errors = ImmutableArray.CreateBuilder<string>();
        if (patch.Operators.IsDefault || patch.Operators.Length != 6 || patch.Operators.Any(o => o is null || o.Envelope is null))
            errors.Add("Exactly six operators with envelopes are required.");
        if (patch.PitchEnvelope is null) errors.Add("Pitch envelope is required.");
        if (patch.Name is null || patch.Name.Length > 10 || patch.Name.Any(c => c < 32 || c > 126))
            errors.Add("Device name must contain at most ten printable ASCII characters.");
        if (errors.Count != 0) return errors.ToImmutable();
        var values = PatchCodec.Values(patch);
        for (int i = 0; i < 145; i++)
        {
            int max = i < 126 ? OperatorMax[i % 21] : VoiceMax[i - 126];
            if (values[i] < 0 || values[i] > max) errors.Add($"Voice field {i}: {values[i]} outside 0..{max}.");
        }
        return errors.ToImmutable();
    }
    public static void Require(Patch patch)
    {
        ArgumentNullException.ThrowIfNull(patch);
        var errors = Errors(patch);
        if (!errors.IsEmpty) throw new ArgumentException(string.Join(" ", errors), nameof(patch));
    }
    /// <summary>Explicit lossy local-name conversion; full names belong in draft metadata.</summary>
    public static string DeviceName(string fullName) => new(fullName.Take(10).Select(c => c is >= ' ' and <= '~' ? c : ' ').ToArray());
}
