namespace Sloop.SoundDesign;

/// <summary>Carrier masks copied from fm6_core.c FM6_ALG output-bus rule. Bit 0 = OP1.</summary>
public static class AlgorithmTopology
{
    static readonly int[] Masks = [5, 5, 9, 9, 21, 21, 5, 5, 5, 9, 9, 5, 5, 5, 5, 1, 1, 1, 25, 11, 27, 29, 27, 31, 31, 11, 11, 37, 23, 39, 31, 63];
    public static bool IsCarrier(int algorithm, int operatorNumber)
    {
        if (algorithm is < 1 or > 32 || operatorNumber is < 1 or > 6)
            throw new ArgumentOutOfRangeException();
        return (Masks[algorithm-1] & (1 << (operatorNumber-1))) != 0;
    }
}
