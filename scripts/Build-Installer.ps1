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
    $approved=@{'opengl32.dll'='C23CC5460BED94EFDD516C376CE802A1F164BC55AACC51E8696954B31571DA9D';'runtime.dll'='105122D6784DAD6CB9133A6FE132C595E5756D4002744C2CD5F1DC6ED36FC80A'}
    foreach($name in $approved.Keys) {
        $path=(Resolve-Path -LiteralPath (Join-Path $RuntimeDirectory $name)).Path
        if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $approved[$name]) { throw "Unapproved payload hash for $name. Run runtime and isolated installation checks before updating the approved release hashes." }
        $common+="/resource:$path,$name"
    }
    $common+='/define:MENU_ALPHA'
}
& $compiler @common /target:winexe "/win32manifest:$root\installer\app.manifest" "/out:$build\SSC-Mod-Menu-Setup.exe" @sources
if($LASTEXITCODE) { throw 'Installer compilation failed' }
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










