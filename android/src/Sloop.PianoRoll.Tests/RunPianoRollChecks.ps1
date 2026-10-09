$ErrorActionPreference = 'Stop'
dotnet run --project (Join-Path $PSScriptRoot 'Sloop.PianoRoll.Tests.csproj')
if ($LASTEXITCODE -ne 0) { throw 'Piano roll checks failed.' }
