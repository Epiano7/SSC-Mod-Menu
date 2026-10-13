param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$BuildDirectory,[string]$GameExecutable)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
foreach($name in @('vehicle_controls','vehicle_adapter')) {
 $exe=(Join-Path (Resolve-Path $BuildDirectory) "$name-test.exe").Replace('\','/')
 $source=(Join-Path $PSScriptRoot "${name}_test.cpp").Replace('\','/')
 & $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror $source -o $exe
 if($LASTEXITCODE){throw "$name build failed"}
 if($name -eq 'vehicle_adapter' -and $GameExecutable){& $exe $GameExecutable}else{& $exe}
 if($LASTEXITCODE){throw "$name failed"}
}
