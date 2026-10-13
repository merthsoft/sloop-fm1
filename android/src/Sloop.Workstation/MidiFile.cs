using System.Collections.Immutable;
using System.Text;
using Sloop.Sequencing;

namespace Sloop.Workstation;

public sealed record MidiFileNote(long Start, long Duration, int Pitch, int Velocity);
public sealed record MidiFilePart(string Name, int Channel, ImmutableArray<MidiFileNote> Notes);
public sealed record MidiFileDocument(int Ppq, int Tempo, ImmutableArray<MidiFilePart> Parts, ImmutableArray<string> Warnings);

/// <summary>Bounded Standard MIDI File reader. Original implementation; no web-editor code is copied.</summary>
public static class MidiFile
{
    public const int MaxBytes = 4 * 1024 * 1024;
    public static MidiFileDocument Read(Stream stream)
    {
        using var buffer = new MemoryStream();
        var chunk = new byte[8192]; int count;
        while ((count = stream.Read(chunk)) != 0) {
            if (buffer.Length + count > MaxBytes) throw new InvalidDataException("MIDI file exceeds 4 MB.");
            buffer.Write(chunk, 0, count);
        }
        return Read(buffer.ToArray());
    }
    public static MidiFileDocument Read(byte[] bytes)
    {
        if (bytes.Length > MaxBytes) throw new InvalidDataException("MIDI file exceeds 4 MB.");
        var r = new Reader(bytes, 0, bytes.Length);
        r.Tag("MThd"); int header = r.Length();
        if (header < 6) throw new InvalidDataException("Invalid MIDI header.");
        int format = r.Word(), tracks = r.Word(), ppq = r.Word();
        if (format > 1) throw new InvalidDataException("Choose a format 0 or 1 MIDI file; independent format 2 songs are unsupported.");
        if (tracks < 1 || tracks > 256 || format == 0 && tracks != 1) throw new InvalidDataException("Invalid MIDI track count.");
        if (ppq == 0 || (ppq & 0x8000) != 0) throw new InvalidDataException("Choose MIDI with musical PPQ timing; SMPTE timing is unsupported.");
        r.Skip(header - 6);
        var parts = ImmutableArray.CreateBuilder<MidiFilePart>();
        var warnings = new HashSet<string>();
        var tempos = new List<(long Tick, int Value)>(); int events = 0, noteCount = 0;
        for (int track = 0; track < tracks; track++) {
            r.Tag("MTrk"); int length = r.Length(); var t = r.Slice(length);
            long tick = 0; int running = 0; string name = $"Track {track + 1}";
            var active = new Dictionary<(int Channel, int Pitch), Queue<(long Tick, int Velocity)>>();
            var notes = new List<(int Channel, MidiFileNote Note)>();
            void End(int channel, int pitch) {
                if (active.TryGetValue((channel, pitch), out var queue) && queue.Count != 0) {
                    var onset = queue.Dequeue();
                    notes.Add((channel, new(onset.Tick, Math.Max(1, tick - onset.Tick), pitch, onset.Velocity)));
                }
            }
            while (t.Remaining > 0) {
                if (++events > 250000) throw new InvalidDataException("MIDI file has too many events.");
                tick = checked(tick + t.Vlq());
                if (tick > (long)ppq * 4 * 4096) throw new InvalidDataException("MIDI exceeds 4096 bars.");
                int status = t.Peek();
                if (status >= 128) t.Byte(); else {
                    if (running == 0) throw new InvalidDataException("MIDI running status has no preceding channel event.");
                    status = running;
                }
                if (status == 255) {
                    running = 0;
                    int kind = t.Byte(), size = t.Vlq(); var data = t.Data(size);
                    if (kind == 3) { var text = Encoding.UTF8.GetString(data).Replace('\n', ' ').Replace('\r', ' '); name = text[..Math.Min(80, text.Length)]; }
                    if (kind == 81) {
                        if (size != 3) throw new InvalidDataException("Invalid MIDI tempo event.");
                        int tempo = (data[0] << 16) | (data[1] << 8) | data[2];
                        if (tempo == 0) throw new InvalidDataException("Invalid zero MIDI tempo.");
                        tempos.Add((tick, tempo));
                    }
                    if (kind == 47) { if (size != 0) throw new InvalidDataException("Invalid end-of-track event."); break; }
                    continue;
                }
                if (status is 240 or 247) { running = 0; t.Skip(t.Vlq()); warnings.Add("SysEx is ignored; no patch or device commands are imported."); continue; }
                if (status >= 240) throw new InvalidDataException("Unsupported MIDI system event.");
                running = status;
                int channel = status & 15, kindChannel = status >> 4, a = t.DataByte();
                int b = kindChannel is 12 or 13 ? 0 : t.DataByte();
                if (kindChannel == 9 && b != 0) {
                    if (++noteCount > 20000) throw new InvalidDataException("MIDI exceeds 20,000 notes.");
                    if (!active.TryGetValue((channel, a), out var queue)) active[(channel, a)] = queue = new();
                    queue.Enqueue((tick, b));
                } else if (kindChannel == 8 || kindChannel == 9) End(channel, a);
                else warnings.Add("Controllers, sustain pedal, pitch bend and program changes are ignored; notes use their MIDI note-off lengths.");
            }
            foreach (var pair in active) while (pair.Value.Count != 0) {
                warnings.Add("Notes missing note-off are closed at the end of their track."); End(pair.Key.Channel, pair.Key.Pitch);
            }
            foreach (var group in notes.GroupBy(n => n.Channel).OrderBy(g => g.Key))
                parts.Add(new(name, group.Key, group.Select(n => n.Note).OrderBy(n => n.Start).ThenBy(n => n.Pitch).ToImmutableArray()));
        }
        if (parts.Count == 0) throw new InvalidDataException("This MIDI file contains no notes.");
        var ordered = tempos.OrderBy(t => t.Tick).ToArray();
        int first = ordered.Length > 0 && ordered[0].Tick == 0 ? ordered[0].Value : 500000;
        if (ordered.Any(t => t.Value != first)) warnings.Add("Tempo changes are ignored; preview uses the starting tempo.");
        int bpm = (int)Math.Round(60000000d / first);
        if (bpm is < 30 or > 240) warnings.Add("Preview tempo is limited to 30–240 BPM.");
        return new(ppq, Math.Clamp(bpm, 30, 240), parts.ToImmutable(), warnings.Order().ToImmutableArray());
    }
    public static AppPattern ToApp(MidiFileDocument file, MidiFilePart part, AppPattern source, int channel)
    {
        if (channel is < 0 or > 15) throw new ArgumentOutOfRangeException(nameof(channel));
        long Scale(long tick) => checked((long)Math.Round(tick * (double)source.TicksPerQuarter / file.Ppq, MidpointRounding.AwayFromZero));
        var imported = part.Notes.Select(n => new AppNote(Guid.NewGuid(), source.Id, new(Scale(n.Start)), new(Math.Max(1, Scale(n.Start + n.Duration) - Scale(n.Start))), n.Pitch, n.Velocity, channel)).ToImmutableArray();
        long end = imported.Max(n => n.Start.Value + n.Duration.Value), bar = checked(source.TicksPerQuarter * 4L);
        var after = source with { Revision = Guid.NewGuid(), Length = new(Math.Max(source.Length.Value, checked((end + bar - 1) / bar * bar))),
            Notes = source.Notes.Where(n => n.Channel != channel).Concat(imported).ToImmutableArray() };
        PatternValidation.Validate(after); return after;
    }
    sealed class Reader(byte[] data, int position, int end)
    {
        public int Remaining => end - position;
        public int Peek() { Require(1); return data[position]; }
        public int Byte() { Require(1); return data[position++]; }
        public int DataByte() { int value = Byte(); if (value > 127) throw new InvalidDataException("Invalid MIDI data byte."); return value; }
        public int Word() => (Byte() << 8) | Byte();
        public int Length() { uint value = ((uint)Word() << 16) | (uint)Word(); if (value > int.MaxValue) throw new InvalidDataException("Invalid MIDI chunk length."); return (int)value; }
        public int Vlq() { int value = 0; for (int i = 0; i < 4; i++) { int b = Byte(); value = (value << 7) | (b & 127); if (b < 128) return value; } throw new InvalidDataException("MIDI variable-length value exceeds four bytes."); }
        public void Tag(string tag) { foreach (char c in tag) if (Byte() != c) throw new InvalidDataException("Invalid MIDI chunk header."); }
        public void Skip(int size) { Require(size); position += size; }
        public byte[] Data(int size) { Require(size); var result = data.AsSpan(position, size).ToArray(); position += size; return result; }
        public Reader Slice(int size) { Require(size); var result = new Reader(data, position, position + size); position += size; return result; }
        void Require(int size) { if (size < 0 || size > Remaining) throw new InvalidDataException("Truncated MIDI file."); }
    }
}
