[CmdletBinding()]
param([Parameter(Mandatory)][string]$Zig,[Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'zig-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'zig-local-cache'
& $Zig cc -target x86_64-linux-musl -Os -static -Wall -Wextra -Werror "$root/linux/launcher.c" -o "$build/SSC-Mod-Menu-Setup"
if($LASTEXITCODE){throw 'Native Linux launcher compilation failed'}
Get-FileHash "$build/SSC-Mod-Menu-Setup"
