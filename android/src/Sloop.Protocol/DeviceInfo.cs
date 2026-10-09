namespace Sloop.Protocol;

public sealed record DeviceInfo(string Firmware, int EngineCount, int ParameterCount,
    int GlobalCount, int StepCount, int EngineParameterStart, IReadOnlyList<string> Engines,
    int TrackCount, int ProtocolVersion)
{
    public static DeviceInfo Parse(EditorFrame frame)
    {
        if (frame.Command != 1) throw new FormatException("Expected INFO reply.");
        var reader = new PayloadReader(frame.Arguments);
        var firmware = reader.String();
        var engines = reader.Byte();
        var parameters = reader.Byte();
        var globals = reader.Byte();
        var steps = reader.Byte();
        var engineStart = reader.Byte();
        if (string.IsNullOrWhiteSpace(firmware) || engines == 0 || parameters == 0 ||
            steps == 0 || engineStart >= parameters)
            throw new FormatException("Invalid INFO capabilities.");
        var names = new string[engines];
        for (var i = 0; i < engines; i++) names[i] = reader.String();
        var tracks = reader.Remaining > 0 ? reader.Byte() : 1;
        var version = reader.Remaining > 0 ? reader.Byte() : tracks > 1 ? 3 : 1;
        if (tracks == 0) throw new FormatException("Invalid track count.");
        return new(firmware, engines, parameters, globals, steps, engineStart,
            Array.AsReadOnly(names), tracks, version);
    }
}

public sealed record ParameterDescriptor(byte Scope, byte Id, byte Format, int Minimum,
    int Maximum, int Default, string Label, string Unit, IReadOnlyList<string> Choices)
{
    public static ParameterDescriptor Parse(EditorFrame frame)
    {
        if (frame.Command != 5) throw new FormatException("Expected DESC reply.");
        var reader = new PayloadReader(frame.Arguments);
        var scope = reader.Byte(); var id = reader.Byte(); var format = reader.Byte();
        var minimum = reader.Value(); var maximum = reader.Value(); var fallback = reader.Value();
        var label = reader.String(); var unit = reader.String();
        if (scope > 1 || minimum > maximum || fallback < minimum || fallback > maximum)
            throw new FormatException("Invalid parameter descriptor.");
        var choices = new List<string>();
        while (reader.Remaining > 0) choices.Add(reader.String());
        return new(scope, id, format, minimum, maximum, fallback, label, unit, choices.AsReadOnly());
    }
}
