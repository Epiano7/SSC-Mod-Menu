[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,
      [Parameter(Mandatory)][string]$Python,
      [Parameter(Mandatory)][string]$PrivateSourceDirectory,
      [string]$BuildDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $BuildDirectory){$BuildDirectory=Join-Path $root 'build/runtime'}
# The private component is an explicit, digest-pinned build dependency. Never
# silently drop the report/collector or regenerate its pins to make a build pass.
& $Python "$PSScriptRoot/verify_public_source.py" --private-source $PrivateSourceDirectory
if($LASTEXITCODE){throw 'Source inventory or private input validation failed'}
New-Item -ItemType Directory -Force -Path $BuildDirectory|Out-Null
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$private=(Resolve-Path -LiteralPath $PrivateSourceDirectory).Path
$authCrypto=& "$PSScriptRoot/Build-AuthCrypto.ps1" -BuildDirectory $build
$audioCodec=& "$PSScriptRoot/Build-AudioCodec.ps1" -Compiler $Compiler -BuildDirectory $build
$auth=@("-I$authCrypto/include",'-DSODIUM_STATIC',"$authCrypto/lib/libsodium.a",'-lwinhttp','-lcrypt32')
$includes=@("-I$root/runtime","-I$private/runtime","-ffile-prefix-map=$root=.","-ffile-prefix-map=$private=.","-ffile-prefix-map=$($root.Replace('\','/'))=.","-ffile-prefix-map=$($private.Replace('\','/'))=.")
# MSYS toolchains may canonicalize C:/Users/NAME to /home/NAME in __FILE__.
$includes+=@("-ffile-prefix-map=$(($root.Replace('\','/') -replace '^[A-Za-z]:/Users/','/home/'))=.","-ffile-prefix-map=$(($private.Replace('\','/') -replace '^[A-Za-z]:/Users/','/home/'))=.")
& $Python "$PSScriptRoot/generate_proxy.py" "$env:WINDIR/System32/opengl32.dll" $build
if($LASTEXITCODE){throw 'Proxy generation failed'}
& $Compiler @includes -O2 -ffp-contract=off -std=c++17 -shared -static -Wall -Wextra -Werror -Wno-cast-function-type "-I$build" "$root/runtime/proxy_loader.cpp" "$build/proxy_stubs.S" "$build/proxy.def" @auth -lbcrypt -lole32 -lwindowscodecs -o "$build/opengl32.dll"
if($LASTEXITCODE){throw 'Proxy compilation failed'}
& $Compiler @includes -O2 -ffp-contract=off -fno-devirtualize -std=c++17 -shared -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/runtime/overlay.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" @auth -o "$build/runtime.dll" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE){throw 'Runtime compilation failed'}
Get-FileHash -LiteralPath "$build/opengl32.dll","$build/runtime.dll" -Algorithm SHA256
