// SPDX-License-Identifier: GPL-3.0-only
using System.Text.Json;
using System.Buffers.Binary;
using Sloop.SampleEncoding;
int checks = 0;
void Check(bool value, string label) { checks++; if (!value) throw new Exception(label); }
void Reject(Action action, string label) { try { action(); } catch (ArgumentException) { checks++; return; } throw new Exception(label); }
using var fixtures = JsonDocument.Parse(File.ReadAllText(Path.Combine(AppContext.BaseDirectory, "Fixtures", "golden.json")));
foreach (var f in fixtures.RootElement.EnumerateArray())
{
    var zones = f.GetProperty("zones").EnumerateArray().Select(z => new ZoneInput(z.GetProperty("pcm").EnumerateArray().Select(s => s.GetInt16()).ToArray(), z.GetProperty("root").GetInt32(), Optional(z,"lo"), Optional(z,"hi"), Optional(z,"ls"), Optional(z,"le"))).ToArray();
    var result = SlotBuilder.Build(f.GetProperty("name").GetString()!, zones);
    string label = f.GetProperty("label").GetString()!;
    Check(result.Header.SequenceEqual(Convert.FromHexString(f.GetProperty("header").GetString()!)), label + " header");
    Check(result.Data.SequenceEqual(Convert.FromHexString(f.GetProperty("data").GetString()!)), label + " data");
    Check(result.Image.AsSpan(512).SequenceEqual(result.Data), label + " image");
    foreach (var preview in result.Preview)
    {
        Check(preview.DecodedPcm.Length == zones[preview.InputIndex].Pcm.Length, label + " preview length");
        Check(preview.DecodedPcm.SequenceEqual(f.GetProperty("zones")[preview.InputIndex].GetProperty("decoded").EnumerateArray().Select(v=>v.GetInt16())),label+" decoded oracle");
        var z = zones[preview.InputIndex];
        if (z.LoopStart is int ls)
        {
            int j = result.Preview.ToList().IndexOf(preview), p = 32 + j*28;
            var state = new AdpcmState(BinaryPrimitives.ReadInt16LittleEndian(result.Header.AsSpan(p+22)), result.Header[p+24]);
            var loop = ImaAdpcm.Decode(result.Data.AsSpan(preview.DataOffset), z.LoopEndExclusive!.Value-ls, state, ls);
            Check(loop.SequenceEqual(preview.DecodedPcm.Skip(ls).Take(loop.Length)), "loop restore");
        }
    }
}
Check(SlotBuilder.Measure([162816]).Fits, "exact fit");
Check(!SlotBuilder.Measure([162817]).Fits, "overflow fit");
Check(SlotBuilder.Measure([1,1]).DataBytes == 2, "per-zone rounding");
Reject(() => SlotBuilder.Build("OVER", [new(new short[162817],60)]), "overflow build");
Reject(() => SlotBuilder.Build("BAD", [new(new short[3],60,LoopStart:2,LoopEndExclusive:2)]), "empty loop");
Reject(() => SlotBuilder.Build("BAD", [new(new short[3],60,LowNote:1)]), "partial keys");
Reject(() => SlotBuilder.Build("BAD", [new(new short[3],128)]), "root");
Reject(() => SlotBuilder.Measure([]), "empty");
Reject(() => SlotBuilder.Measure(Enumerable.Repeat(1,17)), "17 zones");
double[] source = [1,-1,0.5,-0.5]; var original = source.ToArray();
var mixed = PcmConversion.Convert(source,2,22050,new(MonoChoice.AverageChannels,1));
Check(mixed.Samples.All(v=>v==0) && mixed.Report.CancellationFrames == 2, "cancellation");
var first = PcmConversion.Convert(source,2,22050,new(MonoChoice.FirstChannel,1));
Check(first.Samples.SequenceEqual(new short[]{32767,16384}) && first.Report.ClippedSamples==1, "signed scale/clipping");
Check(source.SequenceEqual(original), "immutable PCM");
Reject(() => PcmConversion.Convert([double.NaN],1,22050,new(MonoChoice.FirstChannel,1)), "NaN");
Reject(() => PcmConversion.Convert([0.0],1,22050,new(MonoChoice.FirstChannel,-1)), "gain");
Reject(() => PcmConversion.Convert(Enumerable.Repeat(1.0,100).ToArray(),1,44100,new(MonoChoice.FirstChannel,1e308)), "resampler gain overflow");
var dc = PcmConversion.Convert(Enumerable.Repeat(0.25,4800).ToArray(),1,48000,new(MonoChoice.FirstChannel,1));
Check(dc.Samples.Length==2205 && dc.Samples.All(v=>Math.Abs(v-8192)<=1), "DC resampling");
double[] Tone(double hz) => Enumerable.Range(0,48000).Select(n=>0.8*Math.Sin(2*Math.PI*hz*n/48000)).ToArray();
double Rms(short[] samples) => Math.Sqrt(samples.Skip(100).Take(samples.Length-200).Average(v=>(double)v*v))/32768;
var pass = PcmConversion.Convert(Tone(1000),1,48000,new(MonoChoice.FirstChannel,1));
var stop = PcmConversion.Convert(Tone(18000),1,48000,new(MonoChoice.FirstChannel,1));
Check(Rms(pass.Samples)>0.55 && Rms(stop.Samples)<0.001, "band limit");
Check(PcmConversion.MapBoundary(4800,48000)==2205,"loop boundary mapping");
Check(SlotBuilder.Crc32("123456789"u8)==0xcbf43926,"CRC vector");
var convertedKit = ConversionPipeline.Build("PIPE",[new(new double[480],1,48000,new(MonoChoice.FirstChannel,1),60,LoopStartFrame:0,LoopEndFrameExclusive:480)]);
Check(convertedKit.Artifact.Fit.Samples == 221 && convertedKit.Artifact.Fit.DataBytes==111,"pipeline odd resampling fit");
Check(BinaryPrimitives.ReadUInt32LittleEndian(convertedKit.Artifact.Header.AsSpan(44))==220,"inclusive loop end mapping");
Reject(()=>ConversionPipeline.ConvertZone(new(new double[100],1,48000,new(MonoChoice.FirstChannel,1),60,LoopStartFrame:1,LoopEndFrameExclusive:2)),"collapsed loop");
Console.WriteLine($"Passed {checks} assertions, {fixtures.RootElement.GetArrayLength()} Python golden fixtures.");
static int? Optional(JsonElement z,string key) => z.TryGetProperty(key,out var v) && v.ValueKind!=JsonValueKind.Null ? v.GetInt32() : null;
