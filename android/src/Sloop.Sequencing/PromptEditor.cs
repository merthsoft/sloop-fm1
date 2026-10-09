using System.Globalization;
using System.Text.RegularExpressions;

namespace Sloop.Sequencing;

/// <summary>Exact offline commands over an explicitly captured selection. No inference or device effects.</summary>
public static class PromptEditor
{
    public const string RecipeVersion = "pattern-phrases/1";
    public const string AppCommands = "transpose up/down N semitones; move earlier/later N ticks; " +
        "resize longer/shorter N ticks; quantize N ticks; velocity up/down N; simplify";
    public const string HardwareCommands = "transpose up/down N semitones; move earlier/later N steps; " +
        "velocity up/down N; quantize microtiming; level normal/ghost/soft/hard; simplify";

    private sealed record Command(string Kind, int Amount = 0);
    private static Command Parse(string prompt, bool hardware)
    {
        ArgumentNullException.ThrowIfNull(prompt);
        if (prompt.Length > 256) throw new EditException("Offline commands must be at most 256 characters.");
        var text = Regex.Replace(prompt.Trim().ToLowerInvariant(), @"\s+", " ");
        if (text == "simplify") return new("simplify", 2);
        if (hardware && text == "quantize microtiming") return new("micro");
        if (hardware && Regex.IsMatch(text, @"^level (normal|ghost|soft|hard)$"))
            return new("level", (int)Enum.Parse<HitLevel>(text[6..], true));
        var match = Regex.Match(text, @"^(transpose (up|down) ([0-9]+) semitones|move (earlier|later) ([0-9]+) (ticks|steps)|resize (longer|shorter) ([0-9]+) ticks|quantize ([0-9]+) ticks|velocity (up|down) ([0-9]+))$");
        if (!match.Success) throw new EditException("Unsupported offline command. Use exactly one listed command; extra clauses and preservation wording are rejected. Set selection and locks explicitly.");
        var words = text.Split(' ');
        var number = words[0] == "quantize" ? words[1] : words[2];
        if (!int.TryParse(number, NumberStyles.None, CultureInfo.InvariantCulture, out int amount) || amount <= 0)
            throw new EditException("Amount must be a positive 32-bit integer.");
        if (words[0] == "transpose" && amount > 127 || words[0] == "velocity" && amount > 126)
            throw new EditException("Pitch/velocity amount exceeds the MIDI range.");
        if (words[0] == "move" && words[3] != (hardware ? "steps" : "ticks") ||
            hardware && words[0] is "resize" or "quantize")
            throw new EditException("Command uses timing units or duration unsupported by this target.");
        if (words[1] is "down" or "earlier" or "shorter") amount = -amount;
        return new(words[0], amount);
    }

    public static EditProposal Propose(AppPattern source, AppSelection selection, string prompt,
        EditLocks? locks = null, int seed = 0, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var command = Parse(prompt, false);
        if (!source.Notes.Any(selection.Includes)) throw new EditException("Select at least one note before prompting.");
        var edit = command.Kind switch {
            "transpose" => AppEdit.Transpose(command.Amount), "move" => AppEdit.Move(new(command.Amount)),
            "resize" => AppEdit.Resize(new(command.Amount)), "quantize" => AppEdit.Quantize(new(command.Amount)),
            "velocity" => AppEdit.Velocity(command.Amount), "simplify" => AppEdit.Variation(2, seed),
            _ => throw new EditException("Unsupported app command.")
        };
        var proposal = AppEditor.Propose(source, selection, edit, locks, prompt);
        cancellationToken.ThrowIfCancellationRequested();
        return Stamp(proposal, prompt, command.Kind == "simplify" ? seed : null);
    }

    public static EditProposal Propose(HardwarePattern source, HardwareSelection selection, string prompt,
        EditLocks? locks = null, int seed = 0, CancellationToken cancellationToken = default)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var command = Parse(prompt, true);
        if (selection.DrumLanes is { Count: 0 }) throw new EditException("Select at least one drum lane.");
        var edit = command.Kind switch {
            "transpose" => HardwareEdit.Transpose(command.Amount), "move" => HardwareEdit.Move(new(command.Amount)),
            "velocity" => HardwareEdit.Velocity(command.Amount), "micro" => HardwareEdit.QuantizeMicro(),
            "level" => HardwareEdit.SetLevel((HitLevel)command.Amount), "simplify" => HardwareEdit.Variation(2, seed),
            _ => throw new EditException("Unsupported hardware command.")
        };
        var proposal = HardwareEditor.Propose(source, selection, edit, locks, prompt);
        cancellationToken.ThrowIfCancellationRequested();
        return Stamp(proposal, prompt, command.Kind == "simplify" ? seed : null);
    }

    private static EditProposal Stamp(EditProposal proposal, string prompt, int? seed) =>
        EditProposal.Between(proposal.Before, proposal.After,
            new("Offline command: " + prompt, RecipeVersion, seed, prompt));
}
