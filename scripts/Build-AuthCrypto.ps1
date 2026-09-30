param([Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$dest=Join-Path $BuildDirectory 'auth-crypto'
New-Item -ItemType Directory -Force $dest | Out-Null
$archive=Join-Path $dest 'libsodium-1.0.22-mingw.tar.gz'
$expected='1D99E0AFAF27BCE664249232E9DC628AE6BB7B49F0AB53AD6DB372B028A35D9D'
if(-not (Test-Path -LiteralPath $archive)){Invoke-WebRequest 'https://download.libsodium.org/libsodium/releases/libsodium-1.0.22-mingw.tar.gz' -OutFile $archive}
if((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $expected){throw 'libsodium archive checksum mismatch'}
Push-Location $dest
try {
 & tar -xzf (Split-Path $archive -Leaf) libsodium-win64
 if($LASTEXITCODE){throw 'libsodium extraction failed'}
} finally {Pop-Location}
Write-Output (Join-Path $dest 'libsodium-win64')
