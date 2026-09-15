param([Parameter(Mandatory)][string]$TestDirectory)
$ErrorActionPreference='Stop'
$fixture=Join-Path $TestDirectory 'unknown-build-fixture'
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
[IO.File]::WriteAllText((Join-Path $fixture 'SkillshotCity.exe'),'Not a game executable. Unknown-build rejection fixture.')
try {
    & (Join-Path $PSScriptRoot '..\scripts\Inspect-Game.ps1') -GamePath $fixture
    throw 'Unknown executable was accepted'
} catch {
    if($_.Exception.Message -notlike 'Unknown executable revision*') { throw }
}
Write-Output 'PASS: unknown executable rejected'
