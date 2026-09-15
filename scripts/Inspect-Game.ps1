[CmdletBinding()]
param([string]$GamePath='')
$ErrorActionPreference='Stop'
if (-not $GamePath) {
    $libraries=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($key in @('HKCU:\Software\Valve\Steam','HKLM:\SOFTWARE\WOW6432Node\Valve\Steam')) {
        if (-not (Test-Path -LiteralPath $key)) { continue }
        $properties=Get-ItemProperty -LiteralPath $key
        foreach($name in @('SteamPath','InstallPath')) {
            $property=$properties.PSObject.Properties[$name]
            if($property -and $property.Value) { [void]$libraries.Add($property.Value) }
        }
    }
    foreach($root in @($libraries)) {
        $vdf=Join-Path $root 'steamapps\libraryfolders.vdf'
        if(Test-Path -LiteralPath $vdf) {
            foreach($match in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw),'"path"\s+"([^"]+)"')) {
                [void]$libraries.Add($match.Groups[1].Value.Replace('\\','\'))
            }
        }
    }
    foreach($library in $libraries) {
        $manifest=Join-Path $library 'steamapps\appmanifest_308600.acf'
        if(-not (Test-Path -LiteralPath $manifest)) { continue }
        $acf=Get-Content -LiteralPath $manifest -Raw
        if($acf -notmatch '"appid"\s+"308600"') { continue }
        if($acf -match '"installdir"\s+"([^"]+)"') {
            $directory=$Matches[1]
            if($directory -match '[\\/:]' -or $directory -in @('.','..')) { throw 'Invalid Steam installation directory' }
            $candidate=Join-Path $library "steamapps\common\$directory"
            if(Test-Path -LiteralPath (Join-Path $candidate 'SkillshotCity.exe')) { $GamePath=$candidate; break }
        }
    }
    if(-not $GamePath) { throw 'Steam app 308600 was not found in registry/library manifests.' }
}
$exe=Get-Item -LiteralPath (Join-Path $GamePath 'SkillshotCity.exe')
$hash=(Get-FileHash -LiteralPath $exe.FullName -Algorithm SHA256).Hash
$supported=$exe.Length -eq 15221248 -and $hash -eq '34D8809E646C36E595FDB9DF072E09B31EA35108DEFF3B32EF40451695D05307'
if(-not $supported) { throw "Unknown executable revision. No hooks or installation permitted. SHA256=$hash" }
[pscustomobject]@{Executable=$exe.FullName;Size=$exe.Length;SHA256=$hash;RuntimeSupported=$true}
