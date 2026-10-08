[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,
      [Parameter(Mandatory)][string]$BuildDirectory,
      [Parameter(Mandatory)][string]$AuthCrypto,
      [Parameter(Mandatory)][string]$AudioCodec)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$auth=@("-I$AuthCrypto/include",'-DSODIUM_STATIC',"$AuthCrypto/lib/libsodium.a",'-lwinhttp','-lcrypt32')
$graphics=@('-lopengl32','-lgdi32','-lbcrypt','-lcomdlg32',$AudioCodec,'-lmfreadwrite','-lmfplat','-lmfuuid','-lole32','-lshell32','-lwindowscodecs')
foreach($name in @('statistics_core','local_recorder','tracking_recording','statistics_menu')) {
    $extra=@('-lbcrypt')
    $bridges=@()
    if($name -eq 'tracking_recording'){$bridges=@("$root/runtime/score_bridge.S")}
    if($name -eq 'statistics_menu'){$bridges=@("$root/runtime/score_bridge.S","$root/runtime/hud_bridge.S");$extra=$graphics}
    $exe=Join-Path $build "$name-test.exe"
    & $Compiler "-ffile-prefix-map=$root=." -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/scripts/${name}_test.cpp" @bridges @auth @extra -o $exe
    if($LASTEXITCODE){throw "$name compilation failed"}
    & $exe (Join-Path $build ("$name-fixture-"+[guid]::NewGuid().ToString('N')))
    if($LASTEXITCODE){throw "$name tests failed"}
}
