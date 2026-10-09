$ErrorActionPreference = 'Stop'
$oldAppData = $env:APPDATA
$oldCliHome = $env:DOTNET_CLI_HOME
try {
    $env:APPDATA = Join-Path $PSScriptRoot '.build-profile'
    $env:DOTNET_CLI_HOME = $env:APPDATA
    $project = Join-Path $PSScriptRoot 'Sloop.Sequencing.Tests.csproj'
    dotnet restore $project --configfile (Join-Path $PSScriptRoot 'NuGet.Config') -p:NuGetAudit=false --nologo
    if ($LASTEXITCODE -ne 0) { throw 'Restore failed.' }
    dotnet run --project $project --no-restore
    if ($LASTEXITCODE -ne 0) { throw 'Sequencing tests failed.' }
} finally {
    $env:APPDATA = $oldAppData
    $env:DOTNET_CLI_HOME = $oldCliHome
}
