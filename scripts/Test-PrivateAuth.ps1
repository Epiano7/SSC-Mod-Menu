param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$BuildDirectory,[string]$GameExecutable='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not(Test-Path "$root/tests/private_auth_test.cpp")){throw 'The private local test suite is not present'}
New-Item -ItemType Directory -Force $BuildDirectory | Out-Null
$build=(Resolve-Path $BuildDirectory).Path
$crypto=& "$PSScriptRoot/Build-AuthCrypto.ps1" -BuildDirectory $build
$flags=@('-std=c++17','-O2','-static','-Wall','-Wextra','-Werror','-Wno-cast-function-type','-DSODIUM_STATIC',"-I$crypto/include")
$libs=@("$crypto/lib/libsodium.a",'-lwinhttp','-lcrypt32')
& $Compiler @flags "$root/tests/private_auth_test.cpp" @libs -o "$build/private_auth_test.exe"
if($LASTEXITCODE){throw 'Authorization test compilation failed'}
& "$build/private_auth_test.exe"
if($LASTEXITCODE){throw 'Authorization tests failed'}
# This DLL is strictly an isolated fixture. Never put it beside a game or package it.
$fixture=Join-Path $build 'steam-fixture'
New-Item -ItemType Directory -Force $fixture | Out-Null
& $Compiler -std=c++17 -O2 -static -shared -Wall -Wextra -Werror "$root/tests/mock_steam_auth.cpp" -o "$fixture/steam_api64.dll"
if($LASTEXITCODE){throw 'Mock Steam compilation failed'}
& $Compiler @flags "$root/tests/private_auth_steam_test.cpp" @libs -o "$build/private_auth_steam_test.exe"
if($LASTEXITCODE){throw 'Steam adapter test compilation failed'}
& "$build/private_auth_steam_test.exe" "$fixture/steam_api64.dll"
if($LASTEXITCODE){throw 'Steam adapter tests failed'}
if($GameExecutable){
 & $Compiler @flags "$root/tests/private_bot_layout_test.cpp" "$root/runtime/score_bridge.S" @libs -o "$build/private_bot_layout_test.exe"
 if($LASTEXITCODE){throw 'Bot layout test compilation failed'}
 & "$build/private_bot_layout_test.exe" $GameExecutable
 if($LASTEXITCODE){throw 'Bot layout test failed'}
}

# V2 device-license protocol and async service; fixtures never ship in runtime.
& $Compiler @flags -fno-devirtualize "$root/tests/private_license_test.cpp" @libs -o "$build/private_license_test.exe"
if($LASTEXITCODE){throw 'Device license test compilation failed'}
& "$build/private_license_test.exe" (Join-Path $build ('device-test-'+[guid]::NewGuid().ToString('N')))
if($LASTEXITCODE){throw 'Device license tests failed'}
