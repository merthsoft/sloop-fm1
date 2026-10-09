namespace Sloop.Sequencing;
public static class PatternValidation
{
    public static void Validate(Pattern pattern)
    {
        var ids = new HashSet<Guid>();
        void Identity(Guid id) { if (id == Guid.Empty || !ids.Add(id)) throw new EditException("Identities must be nonempty and unique."); }
        Identity(pattern.Id);
        if (pattern.Revision == Guid.Empty) throw new EditException("Missing revision.");
        switch (pattern)
        {
            case AppPattern app:
                if (app.TicksPerQuarter <= 0 || app.Length.Value <= 0 || app.Notes.IsDefault) throw new EditException("Invalid musical time basis.");
                foreach (var n in app.Notes)
                {
                    Identity(n.Id);
                    if (n.PartId == Guid.Empty || n.Start.Value < 0 || n.Duration.Value <= 0 ||
                        n.Start.Value > app.Length.Value || n.Duration.Value > app.Length.Value - n.Start.Value ||
                        n.Pitch is < 0 or > 127 || n.Velocity is < 1 or > 127 || n.Channel is < 0 or > 15)
                        throw new EditException("App note exceeds pitch, velocity, channel or phrase bounds.");
                }
                break;
            case HardwarePattern hardware:
                if (hardware.Tracks.IsDefault || hardware.Tracks.Length != 4) throw new EditException("Firmware requires exactly four tracks.");
                for (int t = 0; t < 4; t++)
                {
                    var track = hardware.Tracks[t]; Identity(track.Id);
                    if (track.IsDrum != (t == 3) || track.Length is < 1 or > 64 || track.Steps.IsDefault || track.Steps.Length != 64 || track.Locks.IsDefault || track.Locks.Length > 24)
                        throw new EditException("Invalid firmware track shape or capacity.");
                    foreach (var step in track.Steps)
                    {
                        Identity(step.Id);
                        if (step.Micro.Value is < -32 or > 31 || (int)step.Fill is < 0 or > 3) throw new EditException("Invalid native micro timing or fill condition.");
                        if (step is SynthStep s && !track.IsDrum)
                        {
                            if (s.Slots.IsDefault || s.Slots.Length != 4 || s.Count is < 0 or > 4 || (int)s.Time is < 0 or > 2 || ((int)s.Flags & ~3) != 0 || s.Velocity is < 0 or > 127)
                                throw new EditException("Invalid synth step.");
                            foreach (var slot in s.Slots) { Identity(slot.Id); if (slot.Pitch is < 0 or > 127) throw new EditException("Pitch out of range."); Hit(slot.Level, slot.Ratchet); }
                        }
                        else if (step is DrumStep d && track.IsDrum)
                        {
                            if (d.Lanes.IsDefault || d.Lanes.Length != 16) throw new EditException("Firmware has sixteen drum lanes.");
                            foreach (var hit in d.Lanes) { Identity(hit.Id); Hit(hit.Level,hit.Ratchet); }
                        }
                        else throw new EditException("Step type differs from track type.");
                    }
                    var keys = new HashSet<(int,int)>();
                    foreach (var l in track.Locks)
                    {
                        Identity(l.Id);
                        if (l.Value is < short.MinValue or > short.MaxValue || l.Step.Value is < 0 or > 63 || !keys.Add((l.Step.Value,l.Parameter)) ||
                            !track.Capabilities.LockableParameters.Contains(l.Parameter) ||
                            !track.Capabilities.ParameterRanges.TryGetValue(l.Parameter,out var range) ||
                            range.Minimum > range.Maximum || l.Value < range.Minimum || l.Value > range.Maximum)
                            throw new EditException("Invalid, duplicate, unknown-range or unrepresentable parameter lock.");
                    }
                }
                break;
            default: throw new EditException("Unsupported pattern type.");
        }
    }
    private static void Hit(HitLevel level,int ratchet)
    { if ((int)level is < 0 or > 3 || ratchet is < 1 or > 4) throw new EditException("Invalid native level/ratchet."); }
}

