param(
    [string]$AndroidSdkPath = "$env:LOCALAPPDATA\Android\Sdk",
    [string]$JavaSdkPath = "$env:LOCALAPPDATA\Android\Jdk",
    [string]$DeviceSerial
)

$ErrorActionPreference = 'Stop'
$taskAdb = Join-Path $AndroidSdkPath 'platform-tools\adb.exe'
if (-not (Test-Path -LiteralPath $taskAdb)) { throw "ADB not found: $taskAdb. Finish Android SDK installation first." }
$taskProject = Join-Path $PSScriptRoot 'Sloop.Android\Sloop.Android.csproj'

# Fresh build processes avoid stale SDK discovery after first-time SDK installation.
& dotnet build $taskProject "-p:AndroidSdkDirectory=$AndroidSdkPath" "-p:JavaSdkDirectory=$JavaSdkPath" --disable-build-servers -nr:false
if ($LASTEXITCODE -ne 0) { throw 'Android build failed; no APK was installed.' }

$taskDevices = & $taskAdb devices
if ($LASTEXITCODE -ne 0) { throw 'ADB could not enumerate devices.' }
$taskAuthorized = @($taskDevices | ForEach-Object {
    if ($_ -match '^(\S+)\s+device$') { $Matches[1] }
})
if ($DeviceSerial) {
    if ($DeviceSerial -notin $taskAuthorized) { throw "Device $DeviceSerial is not connected and authorized. Unlock the phone and allow USB debugging." }
} elseif ($taskAuthorized.Count -eq 1) {
    $DeviceSerial = $taskAuthorized[0]
} else {
    throw 'Connect and authorize one phone, or pass -DeviceSerial with the serial shown by adb devices.'
}
$taskApk = Join-Path $PSScriptRoot 'Sloop.Android\bin\Debug\net10.0-android\org.sloopfm.mobile-Signed.apk'
if (-not (Test-Path -LiteralPath $taskApk)) { throw "Signed APK not found: $taskApk" }
& $taskAdb -s $DeviceSerial install -r $taskApk
if ($LASTEXITCODE -ne 0) { throw 'APK installation failed.' }
Write-Output 'SLOOP Mobile installed. Open it from the phone app drawer.'
