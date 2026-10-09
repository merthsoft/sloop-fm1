# Sampling parallel implementation handoff — 2026-10-07

Completed domain and Android UI integration inside owned files. No shared-shell hooks are
required for the editor: the existing MainActivity partial and sample workspace events already
connect it. No commits, resets, firmware changes or APK installs were performed.

## Files

- Core/Sampling/SampleDocument.cs: monotonic Revision; MoveBoundary, RemoveBoundary,
  SetSliceRange and ReplaceBoundaries. Invalid requests leave state and undo unchanged.
  Both selected-slice edges change atomically, maintaining contiguous, nonempty neighbors.
- Core/Sampling/SamplingTools.cs (new): SampleViewport, TransientProposal,
  streaming cancellable transient detection and viewport peak extraction.
- Android/SampleEditor.cs: existing recording/import/export and transfer-state callbacks
  retained; waveform zoom/pan/full/fit controls, visible-frame cursor slider, selected-chop
  exact frame fields, one-frame nudges, move/remove end-boundary controls, proposal overlay
  and explicit Apply/Discard. Source frame coordinates remain half-open.
- Android/Services/SampleTools.cs (new partial): ProposeChopsAsync, ChopProposal,
  CancelChopAnalysis, ApplyChopProposal, DiscardChopProposal. Busy serializes analysis with
  editing/conversion/capture; proposal document identity and revision reject stale proposals.
- Android/Services/SampleKit.cs: conversion key includes edit Revision; rebuild clears old
  converted result before work so failures cannot reuse it. Existing settings/routes/protocols
  and upload confirmation flow are retained.
- Sloop.SamplingTools.Tests (new standalone console project): 32 checks.
- design docs/SAMPLING.md: implemented behavior and limits updated.

## Validation

`dotnet run --project android/src/Sloop.SamplingTools.Tests/Sloop.SamplingTools.Tests.csproj`
passes 32 checks. Existing Sloop.Workstation.Tests passes 61 integration checks, including
conversion and backup behavior. Android full build succeeds with the installed SDK/JDK paths
below; four existing CS0108 warnings in Fm6Visuals.cs are outside this slice. The final incremental build compiled the conversion-key change successfully; packaging was stopped
after root requested exclusive Android build ownership. Root owns the combined build/deploy.

Build uses `-p:AndroidSdkDirectory=C:/Users/shaun/AppData/Local/Android/Sdk`
and `-p:JavaSdkDirectory=C:/Users/shaun/AppData/Local/Android/Jdk`.
The sandbox build could discover the SDK but failed silently before compilation; the authorized
build outside that restriction succeeded. No dependencies or services were downloaded.

## Root integration

Register `android/src/Sloop.SamplingTools.Tests/Sloop.SamplingTools.Tests.csproj` in the
reserved Sloop.slnx and optionally the shared test workflow. No change to MainActivity.cs,
WorkspaceStyle.cs, Fm1Connection.cs, transport/session code or firmware is needed.
Deployment and physical acceptance remain owned by the originating chat.

## Practical limits / physical acceptance

Proposal preview means orange waveform boundaries; Apply changes the document and normal chop
pads then audition the resulting slices. It does not audition proposed slices before Apply.
Detection is a 2ms rising-peak heuristic with adjustable sensitivity/spacing, not beat analysis.
Strongest candidates are limited to 15; exact manual frame edits can refine every onset.
Zoom envelopes use a cancellable worker and bounded bucket memory, but scan the visible region.
Navigation is explicit buttons and touch cursor dragging, without pinch or direct boundary drag.
Undo history and viewport are process-local; durable source and edit document restore on restart.
Phone touch/accessibility layout, real audio route changes, USB capture, FM1 slot replacement and
round-trip sound were not physically tested. Preserve existing two-step transfer confirmation.

