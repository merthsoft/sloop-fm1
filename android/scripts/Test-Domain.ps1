[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [switch]$NoBuild,
    [string]$LogDirectory
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repository = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source = Join-Path $repository 'android/src'
if (-not $LogDirectory) { $LogDirectory = Join-Path $repository 'build/android-domain-tests' }
$LogDirectory = [IO.Path]::GetFullPath($LogDirectory)
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null

# Discover dedicated executable runners, including new projects not yet in the solution.
$projects = @(Get-ChildItem -LiteralPath $source -Directory -Filter '*.Tests' |
    Sort-Object Name | ForEach-Object {
        $project = Join-Path $_.FullName ($_.Name + '.csproj')
        if (-not (Test-Path -LiteralPath $project)) { throw "Missing runner project: $project" }
        $project
    })
if ($projects.Count -eq 0) { throw 'No domain test runners found.' }

$results = foreach ($project in $projects) {
    $name = [IO.Path]::GetFileNameWithoutExtension($project)
    $log = Join-Path $LogDirectory ($name + '.log')
    $arguments = @('run', '--project', $project, '--configuration', $Configuration, '--no-launch-profile')
    if ($NoBuild) { $arguments += '--no-build' }
    $timer = [Diagnostics.Stopwatch]::StartNew()
    & dotnet @arguments *> $log
    $code = $LASTEXITCODE
    $timer.Stop()
    Write-Host "$name : $(if ($code -eq 0) { 'PASS' } else { 'FAIL' })"
    [pscustomobject]@{ Project = $name; ExitCode = $code; Seconds = [math]::Round($timer.Elapsed.TotalSeconds, 2); Log = $log }
}
$results | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $LogDirectory 'results.json') -Encoding utf8
$failed = @($results | Where-Object ExitCode -ne 0)
if ($failed.Count) {
    foreach ($failure in $failed) {
        Write-Host "Failure details: $($failure.Log)"
        Get-Content -LiteralPath $failure.Log -Tail 20 | Write-Host
    }
    throw "$($failed.Count) of $($projects.Count) domain runners failed."
}
Write-Host "All $($projects.Count) domain runners passed. Logs: $LogDirectory"
