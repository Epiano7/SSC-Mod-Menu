param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not(Test-Path "$root/tests/beta_auth_test.cpp")){throw 'The isolated local beta test suite is not present'}
New-Item -ItemType Directory -Force $BuildDirectory | Out-Null
$build=(Resolve-Path $BuildDirectory).Path
$crypto=& "$PSScriptRoot/Build-AuthCrypto.ps1" -BuildDirectory $build
& $Compiler -std=c++17 -O2 -fno-devirtualize -static -Wall -Wextra -Werror -Wno-cast-function-type -DSODIUM_STATIC "-I$crypto/include" "$root/tests/beta_auth_test.cpp" "$crypto/lib/libsodium.a" -lwinhttp -lcrypt32 -o "$build/beta_auth_test.exe"
if($LASTEXITCODE){throw 'Beta test compilation failed'}
& "$build/beta_auth_test.exe" (Join-Path $build ('storage-'+[guid]::NewGuid().ToString('N')))
if($LASTEXITCODE){throw 'Beta tests failed'}
