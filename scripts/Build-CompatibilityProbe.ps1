[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$Output)
$ErrorActionPreference='Stop'
$parent=Split-Path -Parent ([IO.Path]::GetFullPath($Output))
New-Item -ItemType Directory -Force -Path $parent | Out-Null
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$PSScriptRoot/compatibility_probe.cpp" -o $Output
if($LASTEXITCODE){throw 'Compatibility probe compilation failed'}
