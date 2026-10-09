$ErrorActionPreference = 'Stop'
$taskCheckDir = Join-Path ([IO.Path]::GetTempPath()) ('sloop-kit-checks-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $taskCheckDir | Out-Null
$taskWorkstation = [Security.SecurityElement]::Escape((Join-Path $PSScriptRoot '../Sloop.Workstation/Sloop.Workstation.csproj'))
$taskChecks = [Security.SecurityElement]::Escape((Join-Path $PSScriptRoot 'SampleKitPlanChecks.cs'))
# A temporary runner avoids changing the shared project/solution or its existing entry point.
@"
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><ImplicitUsings>enable</ImplicitUsings><Nullable>enable</Nullable></PropertyGroup>
  <ItemGroup><ProjectReference Include="$taskWorkstation" /><Compile Include="$taskChecks" /></ItemGroup>
</Project>
"@ | Set-Content -LiteralPath (Join-Path $taskCheckDir 'Checks.csproj')
'Console.WriteLine($"Sample kit plan: {SampleKitPlanChecks.Run()} checks passed.");' | Set-Content -LiteralPath (Join-Path $taskCheckDir 'Program.cs')
dotnet run --project (Join-Path $taskCheckDir 'Checks.csproj')
if ($LASTEXITCODE -ne 0) { throw "Sample kit plan checks failed ($LASTEXITCODE)." }
