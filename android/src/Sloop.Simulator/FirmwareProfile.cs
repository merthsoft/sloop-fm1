using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sloop.Simulator;

public sealed record SimulatorParameter(byte Scope, byte Id, string Symbol, byte Format,
    int Minimum, int Maximum, int Default, string Label, string Unit, string[] Choices);

public sealed record FirmwareProfile(string Firmware, byte ProtocolVersion, byte TrackCount,
    byte StepCount, byte ParameterCount, byte GlobalCount, byte EngineParameterStart,
    string[] Engines, SimulatorParameter[] Parameters, Dictionary<string, string> SourceHashes)
{
    public static FirmwareProfile LoadDefault()
    {
        using var stream = typeof(FirmwareProfile).Assembly.GetManifestResourceStream("Sloop.Simulator.FirmwareProfile.json")
            ?? throw new InvalidOperationException("Embedded firmware profile is missing.");
        return JsonSerializer.Deserialize(stream, SimulatorJsonContext.Default.FirmwareProfile)
            ?? throw new FormatException("Invalid firmware profile.");
    }
}

[JsonSerializable(typeof(FirmwareProfile))]
internal partial class SimulatorJsonContext : JsonSerializerContext;
