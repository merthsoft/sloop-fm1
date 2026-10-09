using Sloop.Core.Sampling;

namespace Sloop.Android.Services;

public sealed partial class SampleWorkspace
{
    private TransientProposal? chopProposal;
    private SampleDocument? proposalDocument;
    private CancellationTokenSource? chopAnalysis;
    public TransientProposal? ChopProposal => ReferenceEquals(Document, proposalDocument) && chopProposal?.Revision == Document?.Revision ? chopProposal : null;
    public async Task ProposeChopsAsync(double sensitivity, int spacingMilliseconds)
    {
        if (Busy || Document is not { } doc) return;
        Busy = true; StopPreview(); tapReview = null; chopProposal = null; chopAnalysis = new();
        var token = chopAnalysis.Token;
        long start = doc.Start, end = doc.End, revision = doc.Revision;
        Status = "Finding transients…"; Notify();
        try
        {
            var result = await Task.Run(() => SamplingTools.Detect(doc.Source, start, end, revision, sensitivity, spacingMilliseconds, token));
            token.ThrowIfCancellationRequested();
            if (!ReferenceEquals(Document, doc) || doc.Revision != revision) return;
            chopProposal = result; proposalDocument = doc;
            Status = $"Preview: {result.Boundaries.Count} proposed boundaries. Apply replaces existing chops; Undo restores them.";
        }
        catch (OperationCanceledException) { Status = "Transient analysis cancelled."; }
        catch (Exception error) { Status = "Transient analysis failed: " + error.Message; }
        finally { chopAnalysis.Dispose(); chopAnalysis = null; Busy = false; Notify(); }
    }
    public void CancelChopAnalysis() => chopAnalysis?.Cancel();
    public void DiscardChopProposal() { chopProposal = null; Notify(); }
    public void ApplyChopProposal()
    {
        if (ChopProposal is not { } proposal) return;
        Edit(doc => doc.ReplaceBoundaries(proposal.Boundaries)); chopProposal = null;
    }
}
