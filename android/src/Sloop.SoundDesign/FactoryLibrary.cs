namespace Sloop.SoundDesign;

public static class FactoryLibrary
{
    public const int Count = 8;
    public static Patch Get(int index)
    {
        if (index is < 0 or >= Count) throw new ArgumentOutOfRangeException(nameof(index));
        using var stream = typeof(FactoryLibrary).Assembly.GetManifestResourceStream($"Sloop.SoundDesign.Factory.{index}.bin")
            ?? throw new InvalidOperationException("Factory resource missing.");
        using var bytes = new MemoryStream(); stream.CopyTo(bytes);
        return PatchCodec.Unpack(bytes.ToArray());
    }
}
