using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using Sloop.Core.Sampling;
using Sloop.SampleEncoding;

namespace Sloop.Workstation;

public sealed record LibrarySample(Guid SessionId, string SessionName, string Name, double Seconds, int Chops,
    string Identity, SessionAsset[] Files);
public sealed record SampleLibraryIndex(LibrarySample[] Samples, int Rejected, bool Truncated);

/// <summary>Manifest-only, bounded snapshot browsing. Index results are hints: reuse revalidates every byte.</summary>
public sealed class SampleLibrary(string sessionsRoot)
{
    public const int MaximumSessions = 1024, MaximumSamples = 4096;
    public const long MaximumScanBytes = 2L * 1024 * 1024 * 1024;
    static readonly string[] Suffixes = ["", ".edits", ".kitsettings", ".chopaudio"];
    readonly string root = Path.GetFullPath(sessionsRoot);
    static bool AssetError(Exception e) => e is IOException or InvalidDataException or UnauthorizedAccessException or ArgumentException or FormatException or OverflowException or JsonException;
    public Task<SampleLibraryIndex> IndexAsync(CancellationToken token = default) => Task.Run(() => Index(token), token);
    public SampleLibraryIndex Index(CancellationToken token = default)
    {
        var result = new List<LibrarySample>(); int rejected = 0, sessions = 0; long bytes = 0; bool truncated = false;
        token.ThrowIfCancellationRequested();
        if (!Directory.Exists(root)) return new([], 0, false);
        Safe(root);
        foreach (var folder in Directory.EnumerateDirectories(root))
        {
            token.ThrowIfCancellationRequested();
            if (++sessions > MaximumSessions) { truncated = true; break; }
            if (!Guid.TryParseExact(Path.GetFileName(folder), "N", out var id)) continue;
            SessionManifest manifest;
            try { manifest = Manifest(folder, id); }
            catch (Exception e) when (AssetError(e)) { rejected++; continue; }
            foreach (var asset in manifest.Assets.Where(a => a.Path.StartsWith("samples/", StringComparison.Ordinal) &&
                a.Path.EndsWith(".wav", StringComparison.OrdinalIgnoreCase) && a.Path.Count(c => c == '/') == 1))
            {
                token.ThrowIfCancellationRequested();
                var files = Suffixes.Select(s => manifest.Assets.SingleOrDefault(a => a.Path == asset.Path + s)).OfType<SessionAsset>().ToArray();
                long size = files.Sum(a => a.Length);
                if (result.Count >= MaximumSamples || size > MaximumScanBytes - bytes) { truncated = true; break; }
                bytes += size;
                try {
                    foreach (var suffix in Suffixes.Skip(1))
                        if (File.Exists(Path.Combine(folder, asset.Path + suffix)) && !files.Any(f => f.Path == asset.Path + suffix))
                            throw new InvalidDataException("Unarchived sample sidecar.");
                    foreach (var file in files) Verify(folder, file, token);
                    var doc = Validate(Path.Combine(folder, asset.Path));
                    result.Add(new(id, manifest.Name, Path.GetFileName(asset.Path), doc.Source.Frames / (double)doc.Source.SampleRate,
                        doc.Slices().Length, Identity(files), files));
                } catch (Exception e) when (AssetError(e)) { rejected++; }
            }
            if (truncated) break;
        }
        return new(result.OrderBy(a => a.SessionName, StringComparer.OrdinalIgnoreCase).ThenBy(a => a.Name).ToArray(), rejected, truncated);
    }
    static string Identity(IEnumerable<SessionAsset> files) => Convert.ToHexString(SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(
        string.Join("\n", files.Select(a => Path.GetExtension(a.Path) + ":" + a.Length + ":" + a.Sha256)))));
    SessionManifest Manifest(string folder, Guid id)
    {
        Safe(root); Safe(folder); var path = Path.Combine(folder, "manifest.json"); Safe(path);
        if (new FileInfo(path).Length > 1024 * 1024) throw new InvalidDataException("Manifest too large.");
        var m = JsonSerializer.Deserialize<SessionManifest>(File.ReadAllBytes(path)) ?? throw new InvalidDataException("Missing manifest.");
        if (m.Schema != 1 || m.Id != id || string.IsNullOrWhiteSpace(m.Name) || m.Name.Length > 80 || m.Name.Any(char.IsControl) ||
            m.Assets is null || m.Assets.Length > SessionStore.MaximumEntries) throw new InvalidDataException("Invalid snapshot manifest.");
        var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase); long total = 0;
        foreach (var a in m.Assets) {
            if (a is null || a.Path is null) throw new InvalidDataException("Null asset.");
            SessionStore.RequirePath(a.Path);
            if (!seen.Add(a.Path) || a.Path.Equals("manifest.json", StringComparison.OrdinalIgnoreCase) || a.Length < 0 ||
                a.Length > SessionStore.MaximumAssetBytes || (total += a.Length) > SessionStore.MaximumTotalBytes ||
                a.Sha256 is null || a.Sha256.Length != 64 || !a.Sha256.All(Uri.IsHexDigit)) throw new InvalidDataException("Invalid asset metadata.");
        }
        return m;
    }
    static void Safe(string path) { if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Linked library assets are not supported."); }
    static void Verify(string folder, SessionAsset asset, CancellationToken token)
    {
        string path = Path.Combine(folder, asset.Path); Safe(Path.GetDirectoryName(path)!); Safe(path);
        if (new FileInfo(path).Length != asset.Length || Hash(path, token) != asset.Sha256.ToUpperInvariant()) throw new InvalidDataException("Sample source changed or is missing. Refresh the library.");
    }
    static string Hash(string path, CancellationToken token)
    {
        using var stream = File.OpenRead(path); using var hash = IncrementalHash.CreateHash(HashAlgorithmName.SHA256);
        var buffer = new byte[65536]; int n;
        while ((n = stream.Read(buffer)) != 0) { token.ThrowIfCancellationRequested(); hash.AppendData(buffer, 0, n); }
        token.ThrowIfCancellationRequested(); return Convert.ToHexString(hash.GetHashAndReset());
    }
    public static SampleDocument Validate(string path)
    {
        Safe(Path.GetDirectoryName(path)!); Safe(path);
        if (new FileInfo(path).Length > SessionStore.MaximumAssetBytes) throw new InvalidDataException("WAV too large.");
        foreach (var suffix in Suffixes.Skip(1)) if (File.Exists(path + suffix)) {
            Safe(path + suffix); long maximum = suffix == ".chopaudio" ? 16384 : 4096;
            if (new FileInfo(path + suffix).Length > maximum) throw new InvalidDataException("Sample sidecar too large.");
        }
        var wave = PcmWave.Open(path);
        // Require exact RIFF extent; trailing/truncated bytes cannot silently change source identity.
        using (var reader = new BinaryReader(File.OpenRead(path))) {
            reader.BaseStream.Position = 4;
            if (8L + reader.ReadUInt32() != reader.BaseStream.Length) throw new InvalidDataException("WAV extent differs from RIFF length.");
            reader.BaseStream.Position = 12; int formats = 0, data = 0;
            while (reader.BaseStream.Position < reader.BaseStream.Length) {
                if (reader.BaseStream.Length - reader.BaseStream.Position < 8) throw new InvalidDataException("Partial WAV chunk.");
                var tag = System.Text.Encoding.ASCII.GetString(reader.ReadBytes(4)); long size = reader.ReadUInt32();
                if (tag == "fmt ") formats++; if (tag == "data") data++;
                var next = reader.BaseStream.Position + size + (size & 1);
                if (next > reader.BaseStream.Length) throw new InvalidDataException("Partial WAV chunk padding.");
                reader.BaseStream.Position = next;
            }
            if (formats != 1 || data != 1) throw new InvalidDataException("WAV must have one format and one PCM data chunk.");
        }
        if (File.Exists(path + ".edits")) {
            var boundaries = File.ReadAllLines(path + ".edits").Skip(3).Select(s => long.Parse(s, CultureInfo.InvariantCulture)).ToArray();
            if (!boundaries.SequenceEqual(boundaries.Distinct().Order())) throw new InvalidDataException("Unordered chop boundaries.");
        }
        var doc = File.Exists(path + ".edits") ? SampleDocument.Restore(wave, path + ".edits") : new(wave);
        ChopAudioSettings.ValidateFile(wave); ReadKit(path, doc.Slices().Length);
        return doc;
    }
    public static KitSettings ReadKit(string path, int chops)
    {
        var options = new KitSettings("SLOOP", SampleMapping.Chops, 60, MonoChoice.AverageChannels, 1);
        if (File.Exists(path + ".kitsettings")) {
            var l = File.ReadAllLines(path + ".kitsettings");
            if (l.Length is not (6 or 7) || l[0] != "SLOOP-KIT-1") throw new InvalidDataException("Invalid kit settings.");
            options = new(System.Text.Encoding.UTF8.GetString(Convert.FromBase64String(l[1])), Enum.Parse<SampleMapping>(l[2]),
                int.Parse(l[3], CultureInfo.InvariantCulture), Enum.Parse<MonoChoice>(l[4]), double.Parse(l[5], CultureInfo.InvariantCulture), l.Length == 7 && bool.Parse(l[6]));
        }
        if (options.Name.Length > 80 || options.Name.Any(char.IsControl)) throw new InvalidDataException("Invalid kit name.");
        SampleKitPlan.Validate(options, chops); return options;
    }
    public Task<string> CopyAsync(LibrarySample sample, string destination, CancellationToken token = default, IProgress<long>? progress = null) => Task.Run(() => Copy(sample, destination, token, progress), token);
    public string Copy(LibrarySample sample, string destination, CancellationToken token = default, IProgress<long>? progress = null)
    {
        token.ThrowIfCancellationRequested();
        var folder = Path.Combine(root, sample.SessionId.ToString("N")); var manifest = Manifest(folder, sample.SessionId);
        SessionStore.RequirePath(sample.Name);
        if (Path.GetFileName(sample.Name) != sample.Name || !sample.Name.EndsWith(".wav", StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid sample name.");
        var files = Suffixes.Select(s => manifest.Assets.SingleOrDefault(a => a.Path == "samples/" + sample.Name + s)).OfType<SessionAsset>().ToArray();
        if (files.Length == 0 || files[0].Path != "samples/" + sample.Name || Identity(files) != sample.Identity) throw new InvalidDataException("Sample changed. Refresh the library.");
        foreach (var suffix in Suffixes.Skip(1))
            if (File.Exists(Path.Combine(folder, "samples/" + sample.Name + suffix)) && !files.Any(f => f.Path == "samples/" + sample.Name + suffix))
                throw new InvalidDataException("Unarchived sample sidecar.");
        Directory.CreateDirectory(destination); Safe(destination);
        // Stable name deduplicates the WAV + complete sidecar state, not just audio bytes.
        var name = "library-" + sample.Identity + ".wav"; var final = Path.Combine(destination, name);
        if (File.Exists(final)) {
            foreach (var f in files) Verify(folder, f, token);
            try {
                foreach (var f in files) { var suffix = f.Path[("samples/" + sample.Name).Length..]; Verify(destination, f with { Path = name + suffix }, token); }
                foreach (var suffix in Suffixes.Skip(1)) if (!files.Any(f => f.Path == "samples/" + sample.Name + suffix) && File.Exists(final + suffix)) throw new InvalidDataException("Existing copy settings changed.");
                Validate(final); return name;
            } catch (Exception error) when (AssetError(error)) {
                // Edits to a prior writable copy are user work; preserve them and make a new copy.
                name = "library-" + sample.Identity + "-" + Guid.NewGuid().ToString("N") + ".wav"; final = Path.Combine(destination, name);
            }
        }
        else if (Suffixes.Skip(1).Any(s => File.Exists(final + s))) {
            // A process-killed publication may leave sidecars before its WAV. Never overwrite them.
            name = "library-" + sample.Identity + "-" + Guid.NewGuid().ToString("N") + ".wav"; final = Path.Combine(destination, name);
        }
        var stage = Path.Combine(destination, ".library-stage-" + Guid.NewGuid().ToString("N")); Directory.CreateDirectory(stage);
        var published = new List<string>();
        try {
            foreach (var f in files) {
                Verify(folder, f, token); var suffix = f.Path[("samples/" + sample.Name).Length..]; var target = Path.Combine(stage, name + suffix);
                using var input = File.OpenRead(Path.Combine(folder, f.Path)); using var output = new FileStream(target, FileMode.CreateNew, FileAccess.Write);
                var buffer = new byte[65536]; long bytes = 0; int n;
                while ((n = input.Read(buffer)) != 0) { token.ThrowIfCancellationRequested(); bytes += n; if (bytes > f.Length) throw new InvalidDataException("Source grew during copy."); output.Write(buffer, 0, n); progress?.Report(bytes); }
                output.Flush(true); if (bytes != f.Length) throw new InvalidDataException("Source truncated during copy.");
            }
            foreach (var f in files) Verify(stage, f with { Path = name + f.Path[("samples/" + sample.Name).Length..] }, token);
            Validate(Path.Combine(stage, name)); token.ThrowIfCancellationRequested();
            // Publish original last: partially copied sidecars are never browseable WAV entries.
            foreach (var f in files.Reverse()) {
                token.ThrowIfCancellationRequested(); var target = final + f.Path[("samples/" + sample.Name).Length..];
                File.Move(Path.Combine(stage, Path.GetFileName(target)), target); published.Add(target);
            }
            token.ThrowIfCancellationRequested(); return name;
        } catch { foreach (var path in published) File.Delete(path); throw; }
        finally { Directory.Delete(stage, true); }
    }
}
