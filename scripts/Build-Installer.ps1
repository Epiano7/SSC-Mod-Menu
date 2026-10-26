[CmdletBinding()]
param([string]$BuildDirectory='', [string]$GamePath='', [string]$RuntimeDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $BuildDirectory) { $BuildDirectory=Join-Path $root 'build\installer' }
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if(-not(Test-Path -LiteralPath $compiler)) { throw 'The Windows .NET Framework C# compiler is required.' }
$sources=@("$root\installer\Engine.cs","$root\installer\Setup.cs","$root\installer\Updater.cs")
$common=@('/nologo','/warnaserror','/platform:x64','/r:System.Windows.Forms.dll','/r:System.Drawing.dll','/r:System.Web.Extensions.dll')
if($RuntimeDirectory) {
    # Approved runtime/proxy hashes for the packaged release.
    $approved=@{'opengl32.dll'='31370D5ECBC5ED537FA790E33F117F8C297A8F1065506035C342C5A40215117B';'runtime.dll'='50DDA2F0ED1AE91B85B8D969197EC9A47CD893AAC193D9D046D0C82A4AED94E2'}
    foreach($name in $approved.Keys) {
        $path=(Resolve-Path -LiteralPath (Join-Path $RuntimeDirectory $name)).Path
        if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $approved[$name]) { throw "Unapproved payload hash for $name. Run runtime and isolated installation checks before updating the approved release hashes." }
        $common+="/resource:$path,$name"
    }
    $common+="/resource:$root/runtime/third_party/AUDIO-NOTICES.txt,AUDIO-NOTICES.txt"
    $common+='/define:MENU_ALPHA'
}
& $compiler @common /target:winexe "/win32manifest:$root\installer\app.manifest" "/out:$build\SSC-Mod-Menu-Setup.exe" @sources
if($LASTEXITCODE) { throw 'Installer compilation failed' }
if(-not(Test-Path -LiteralPath "$root/tests")) {
    Write-Host 'Installer built; local test suite is not present'
    Get-FileHash -LiteralPath "$build/SSC-Mod-Menu-Setup.exe" -Algorithm SHA256
    return
}
& $compiler @common /target:exe /main:InstallerTests "/out:$build\installer_test.exe" @sources "$root\tests\installer_test.cs"
if($LASTEXITCODE) { throw 'Installer test compilation failed' }
$testArgs=@((Join-Path $build 'fixtures'))
if($GamePath) { $testArgs+=$GamePath }
& "$build\installer_test.exe" @testArgs
if($LASTEXITCODE) { throw 'Installer tests failed' }
& $compiler @common /target:exe /main:InstallerUiTests "/out:$build\installer_ui_test.exe" @sources "$root\tests\installer_ui_test.cs"
if($LASTEXITCODE) { throw 'Installer UI test compilation failed' }
& "$build\installer_ui_test.exe" (Join-Path $build ('ui-' + [guid]::NewGuid().ToString('N')))
if($LASTEXITCODE) { throw 'Installer UI tests failed' }
& $compiler @common /target:exe /main:UpdaterTests "/out:$build\updater_test.exe" @sources "$root\tests\updater_test.cs"
if($LASTEXITCODE) { throw 'Updater test compilation failed' }
& "$build\updater_test.exe"
if($LASTEXITCODE) { throw 'Updater tests failed' }
Get-FileHash -LiteralPath "$build\SSC-Mod-Menu-Setup.exe" -Algorithm SHA256


