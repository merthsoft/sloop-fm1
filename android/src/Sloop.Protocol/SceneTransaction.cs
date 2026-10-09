namespace Sloop.Protocol;

public enum SceneTransactionOperation : byte { Capabilities, Begin, Data, Prepare, Commit, Cancel, Status }
public enum SceneTransactionCode : byte { Ok, Invalid, Unsupported, Busy, Conflict, Missing, TooLate }
public enum SceneTransactionState : byte { Idle, Receiving, Prepared, Queued, Applied, Canceled, Failed }
public enum SceneMusicalBoundary : byte { Beat, Bar, Phrase }
public enum SceneSampleDependencyState { VerifiedFlashResident, Missing, Unsupported }
public sealed record SceneSampleDependency(string AssetId, SceneSampleDependencyState State);
public sealed record SceneTransactionReply(SceneTransactionOperation Operation, SceneTransactionCode Code,
    SceneTransactionState State, int Epoch, int Token, int Received, int Revision, int Boundary,
    SceneMusicalBoundary BoundaryKind, int MaximumBytes, int ChunkBytes, byte Flags)
{
    public bool SupportsAtomicScene => (Flags & 7) == 7;
}

/// <summary>Canonical v1 schema, independent of firmware struct padding. Owns all synth pattern bytes.</summary>
public static class SceneTransactionCodec
{
    public const byte Command = 72;
    public const int MaximumBytes = 3072, ChunkBytes = 96, SchemaBytes = 2956, TrackBytes = 984;
    public static byte[] Build(int tempo, IReadOnlyList<byte[]> tracks,
        IEnumerable<SceneSampleDependency>? dependencies = null)
    {
        // Flash dependencies must be resolved independently before staging. This schema
        // never carries a flash upload or a drum/sample-track replacement.
        if (dependencies?.Any(d => d.State != SceneSampleDependencyState.VerifiedFlashResident) == true)
            throw new NotSupportedException("Scene has missing or unsupported flash sample dependencies.");
        if (tracks.Count != 3) throw new ArgumentException("Exactly three ordered synth records are required.");
        var bytes = new byte[SchemaBytes]; bytes[0] = 1; Put(bytes.AsSpan(1), tempo, 2); bytes[3] = 7;
        for (int i = 0; i < 3; i++)
        {
            if (tracks[i].Length != TrackBytes) throw new ArgumentException("Invalid native synth record length.");
            tracks[i].CopyTo(bytes, 4 + i * TrackBytes);
        }
        Validate(bytes); return bytes;
    }
    public static void Validate(ReadOnlySpan<byte> p)
    {
        if (p.Length != SchemaBytes || p[0] != 1 || p[1] > 127 || p[2] > 127 ||
            Read(p[1..], 2) is < 20 or > 300 || p[3] != 7) throw new FormatException("Invalid scene header.");
        int o = 4;
        for (int t = 0; t < 3; t++)
        {
            for (int i = 0; i < 142; i++) if (p[o + i] > 127) throw new FormatException("Invalid FM6/macros.");
            o += 142;
            if (p[o] is < 1 or > 64 || p[o + 1] > 8) throw new FormatException("Invalid length/division.");
            o += 2;
            for (int i = 0; i < 64; i++, o += 11)
            {
                for (int j = 0; j < 4; j++) if (p[o + j] > 127) throw new FormatException("Invalid note.");
                if (p[o + 4] > 4 || p[o + 5] > 2 || p[o + 6] > 3 || p[o + 7] > 127 || p[o + 10] > 63)
                    throw new FormatException("Invalid step.");
            }
            for (int i = 0; i < 16; i++, o++) for (int j = 0; j < 4; j++)
                if (((p[o] >> (j * 2)) & 3) == 3) throw new FormatException("Reserved fill condition.");
            for (int i = 0; i < 24; i++, o += 5)
                if ((p[o] != 255 && p[o] >= 64) || p[o + 1] > 127 || p[o + 2] > 127 || p[o + 3] > 127 || p[o + 4] != 0)
                    throw new FormatException("Invalid lock.");
        }
    }
    public static uint Crc(ReadOnlySpan<byte> bytes)
    {
        uint c = uint.MaxValue;
        foreach (var b in bytes) { c ^= b; for (int i = 0; i < 8; i++) c = (c >> 1) ^ ((0u - (c & 1)) & 0xedb88320u); }
        return ~c;
    }
    internal static void Put(Span<byte> output, long value, int count)
    {
        if (value < 0 || value >= (1L << (count * 7))) throw new ArgumentOutOfRangeException(nameof(value));
        for (int i = 0; i < count; i++) output[i] = (byte)((value >> (7 * i)) & 127);
    }
    internal static int Read(ReadOnlySpan<byte> p, int count)
    { int v = 0; for (int i = 0; i < count; i++) { if (p[i] > 127) throw new FormatException("Non MIDI data."); v |= p[i] << (7 * i); } return v; }
    public static SceneTransactionReply Parse(EditorFrame frame)
    {
        var p = frame.Arguments;
        if (frame.Command != Command || p.Length != 24 || p.Any(b => b > 127) || p[0] > 6 || p[1] > 6 || p[2] > 6 || p[19] > 2)
            throw new FormatException("Invalid scene transaction reply.");
        return new((SceneTransactionOperation)p[0], (SceneTransactionCode)p[1], (SceneTransactionState)p[2],
            Read(p.AsSpan(3),4), Read(p.AsSpan(7),2), Read(p.AsSpan(9),2), Read(p.AsSpan(11),4),
            Read(p.AsSpan(15),4), (SceneMusicalBoundary)p[19], Read(p.AsSpan(20),2), p[22], p[23]);
    }
}

/// <summary>Caller supplies a stable connection epoch and monotonically increasing nonzero token.
/// Timeout/disconnect is Unknown; queued acknowledgement is never treated as Applied.</summary>
public sealed class SceneTransactionClient(EditorClient client)
{
    public async Task<SceneTransactionReply> CapabilitiesAsync(int protocolVersion, CancellationToken token = default)
    {
        if (protocolVersion < 11) throw new NotSupportedException("Use the stopped-only sound fallback on protocol 10 or earlier.");
        return SceneTransactionCodec.Parse(await client.RequestAsync(SceneTransactionCodec.Command, [0], cancellationToken: token));
    }
    public async Task<SceneTransactionReply> StageAsync(SceneTransactionReply capabilities, int id, int revision,
        byte[] payload, CancellationToken token = default)
    {
        SceneTransactionCodec.Validate(payload);
        if (!capabilities.SupportsAtomicScene || capabilities.Epoch == 0 || capabilities.MaximumBytes < payload.Length ||
            capabilities.ChunkBytes is < 1 or > SceneTransactionCodec.ChunkBytes)
            throw new NotSupportedException("Atomic scene hooks or staging limits are unavailable.");
        // Copy before the first await so caller edits cannot alter staged bytes/CRC.
        var copy = payload.ToArray();
        byte[] body = new byte[11]; SceneTransactionCodec.Put(body, revision,4);
        SceneTransactionCodec.Put(body.AsSpan(4), copy.Length,2);
        SceneTransactionCodec.Put(body.AsSpan(6), SceneTransactionCodec.Crc(copy),5);
        var reply = await Send(SceneTransactionOperation.Begin, capabilities.Epoch, id, body, token);
        Require(reply, SceneTransactionState.Receiving);
        for (int off = 0; off < copy.Length; off += capabilities.ChunkBytes)
        {
            var part = EditorCodec.Pack7(copy.AsSpan(off, Math.Min(capabilities.ChunkBytes, copy.Length - off)));
            body = new byte[2 + part.Length]; SceneTransactionCodec.Put(body,off,2); part.CopyTo(body,2);
            reply = await Send(SceneTransactionOperation.Data, capabilities.Epoch,id,body,token);
            Require(reply, SceneTransactionState.Receiving);
        }
        reply = await Send(SceneTransactionOperation.Prepare,capabilities.Epoch,id,[],token);
        Require(reply,SceneTransactionState.Prepared); return reply;
    }
    public Task<SceneTransactionReply> CommitAsync(SceneTransactionReply prepared, int absoluteTick, SceneMusicalBoundary boundary, CancellationToken token = default)
    {
        if (prepared.State != SceneTransactionState.Prepared || (byte)boundary > 2) throw new InvalidOperationException("Scene is not prepared.");
        var body = new byte[5]; SceneTransactionCodec.Put(body,absoluteTick,4); body[4]=(byte)boundary;
        return Send(SceneTransactionOperation.Commit,prepared.Epoch,prepared.Token,body,token);
    }
    public Task<SceneTransactionReply> StatusAsync(int epoch,int id,CancellationToken token=default) => Send(SceneTransactionOperation.Status,epoch,id,[],token);
    public Task<SceneTransactionReply> CancelAsync(int epoch,int id,CancellationToken token=default) => Send(SceneTransactionOperation.Cancel,epoch,id,[],token);
    private static void Require(SceneTransactionReply reply,SceneTransactionState state)
    { if(reply.Code!=SceneTransactionCode.Ok || reply.State!=state) throw new IOException($"Scene transaction: {reply.Code}, {reply.State}."); }
    private async Task<SceneTransactionReply> Send(SceneTransactionOperation op,int epoch,int id,byte[] body,CancellationToken token)
    {
        if(id is < 1 or > 16383 || epoch==0) throw new ArgumentOutOfRangeException(nameof(id));
        var args=new byte[7+body.Length]; args[0]=(byte)op; SceneTransactionCodec.Put(args.AsSpan(1),epoch,4);
        SceneTransactionCodec.Put(args.AsSpan(5),id,2); body.CopyTo(args,7);
        var frame=await client.RequestAsync(SceneTransactionCodec.Command,args,
            f=>f.Arguments.Length==24 && f.Arguments[0]==(byte)op,cancellationToken:token);
        var reply=SceneTransactionCodec.Parse(frame);
        if(reply.Epoch!=epoch || reply.Token!=id) throw new IOException("Scene epoch/token conflict; reconcile hardware.");
        return reply;
    }
}
