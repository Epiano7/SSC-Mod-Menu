[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,
      [Parameter(Mandatory)][string]$BuildDirectory,
      [Parameter(Mandatory)][string]$GameExecutable)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
foreach($name in @('freecam','creator_controls')) {
    $exe=Join-Path $build "$name-test.exe"
    & $Compiler "-ffile-prefix-map=$root=." -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/scripts/${name}_test.cpp" -lopengl32 -lgdi32 -o $exe
    if($LASTEXITCODE){throw "$name compilation failed"}
    & $exe $GameExecutable
    if($LASTEXITCODE){throw "$name tests failed"}
}
