[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$RuntimeDirectory,
    [Parameter(Mandatory)][string]$Python,
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$LinuxPythonArchive,
    [Parameter(Mandatory)][string]$LinuxLauncher
)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/Build-Installer.ps1" -RuntimeDirectory $RuntimeDirectory -BuildDirectory $BuildDirectory
& $Python "$PSScriptRoot/Build-Linux.py" --runtime $RuntimeDirectory --python-archive $LinuxPythonArchive --launcher $LinuxLauncher --output (Join-Path $BuildDirectory 'SSC-Mod-Menu-Linux.tar.gz')
if($LASTEXITCODE){throw 'Linux packaging failed'}
Write-Host 'Release assets: SSC-Mod-Menu-Setup.exe and SSC-Mod-Menu-Linux.tar.gz (Linux experimental until Proton validation)'
