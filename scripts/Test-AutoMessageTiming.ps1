[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,
      [Parameter(Mandatory)][string]$BuildDirectory,
      [Parameter(Mandatory)][string]$AuthCrypto)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$exe=Join-Path (Resolve-Path -LiteralPath $BuildDirectory).Path 'auto-message-timing-test.exe'
& $Compiler "-ffile-prefix-map=$root=." -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/scripts/auto_message_timing_test.cpp" "-I$AuthCrypto/include" -DSODIUM_STATIC "$AuthCrypto/lib/libsodium.a" -lwinhttp -lcrypt32 -lbcrypt -o $exe
if($LASTEXITCODE){throw 'Auto-message timing compilation failed'}
& $exe
if($LASTEXITCODE){throw 'Auto-message timing tests failed'}
