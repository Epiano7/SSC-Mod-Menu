param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$BuildDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$vendor=Join-Path $root 'runtime/third_party'
$build=Join-Path $BuildDirectory 'audio-codec'
New-Item -ItemType Directory -Force $build | Out-Null
$build=(Resolve-Path $build).Path
$include=@("-I$vendor/ogg/include","-I$vendor/vorbis/include","-I$vendor/vorbis/lib")
$names='mdct smallft block envelope window lsp lpc analysis synthesis psy info floor1 floor0 res0 mapping0 registry codebook sharedbook lookup bitrate vorbisfile vorbisenc'.Split(' ')
$sources=@("$vendor/ogg/src/bitwise.c","$vendor/ogg/src/framing.c")+@($names|ForEach-Object {"$vendor/vorbis/lib/$_.c"})
$objects=@()
foreach($source in $sources){
 $object=Join-Path $build ((Split-Path $source -Leaf)+'.o')
 & $Compiler -x c -std=gnu99 -O2 -w @include -c $source -o $object
 if($LASTEXITCODE){throw "Audio codec compilation failed: $source"}
 $objects+=$object
}
$facade=Join-Path $build 'audio_codec.o'
& $Compiler -std=c++17 -O2 -Wall -Wextra @include -c "$root/runtime/audio_codec.cpp" -o $facade
if($LASTEXITCODE){throw 'Audio codec facade compilation failed'}
$objects+=$facade
$result=Join-Path $build 'codec.a'
$archiver=& $Compiler -print-prog-name=ar
& $archiver rcs $result @objects
if($LASTEXITCODE){throw 'Audio codec linking failed'}
Write-Output $result
