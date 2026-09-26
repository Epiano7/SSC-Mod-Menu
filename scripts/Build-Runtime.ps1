[CmdletBinding()]
param([Parameter(Mandatory)][string]$Compiler,[Parameter(Mandatory)][string]$Python,[string]$BuildDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $BuildDirectory) {$BuildDirectory=Join-Path $root 'build\runtime'}
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build=(Resolve-Path -LiteralPath $BuildDirectory).Path
$audioCodec=& "$PSScriptRoot/Build-AudioCodec.ps1" -Compiler $Compiler -BuildDirectory $build
& $Python "$PSScriptRoot\generate_proxy.py" "$env:WINDIR\System32\opengl32.dll" $build
if($LASTEXITCODE) {throw 'Proxy generation failed (Python pefile is required)'}
& $Compiler -O2 -std=c++17 -shared -static -Wall -Wextra -Werror "-I$build" "$root/runtime/proxy_loader.cpp" "$build\proxy_stubs.S" "$build\proxy.def" -o "$build\opengl32.dll"
if($LASTEXITCODE) {throw 'Proxy compilation failed'}
& $Compiler -O2 -std=c++17 -shared -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/runtime/overlay.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build\runtime.dll" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Runtime compilation failed'}
if(-not(Test-Path -LiteralPath "$root/tests")) {
    Write-Host 'Runtime built; local test suite is not present'
    Get-FileHash -LiteralPath "$build/runtime.dll" -Algorithm SHA256
    return
}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/menu_hotkey_test.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build\menu_hotkey_test.exe" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Hotkey test compilation failed'}
& "$build\menu_hotkey_test.exe"
if($LASTEXITCODE) {throw 'Hotkey tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/menu_ui_test.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build\menu_ui_test.exe" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Menu UI test compilation failed'}
& "$build\menu_ui_test.exe" "$build\ui-test-state"
if($LASTEXITCODE) {throw 'Menu UI tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/name_adapter_test.cpp" "$root/tests/score_test_call.S" "$root/runtime/score_bridge.S" -o "$build\name_adapter_test.exe"
if($LASTEXITCODE) {throw 'Name adapter test compilation failed'}
& "$build\name_adapter_test.exe"
if($LASTEXITCODE) {throw 'Name adapter tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/audio_module_test.cpp" -o "$build\audio_module_test.exe" -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Audio test compilation failed'}
& "$build\audio_module_test.exe" (Join-Path $build ('audio-test-' + [guid]::NewGuid().ToString('N'))) "$root/tests/fixtures/import-tone.mp3"
if($LASTEXITCODE) {throw 'Audio module tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type -fno-devirtualize "$root/tests/presence_test.cpp" "$root/runtime/score_bridge.S" -o "$build\presence_test.exe"
if($LASTEXITCODE) {throw 'Presence test compilation failed'}
& "$build\presence_test.exe"
if($LASTEXITCODE) {throw 'Presence tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/hud_editor_test.cpp" "$root/runtime/hud_bridge.S" -o "$build\hud_editor_test.exe" -lopengl32 -lgdi32
if($LASTEXITCODE) {throw 'HUD test compilation failed'}
& "$build\hud_editor_test.exe"
if($LASTEXITCODE) {throw 'HUD tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/hud_bridge_test.cpp" "$root/runtime/hud_bridge.S" -o "$build\hud_bridge_test.exe" -lopengl32 -lgdi32
if($LASTEXITCODE) {throw 'HUD bridge test compilation failed'}
& "$build\hud_bridge_test.exe"
if($LASTEXITCODE) {throw 'HUD bridge tests failed'}
Get-FileHash -LiteralPath "$build\opengl32.dll","$build\runtime.dll" -Algorithm SHA256





& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/compatibility_test.cpp" -o "$build/compatibility_test.exe"
if($LASTEXITCODE) {throw 'Compatibility test compilation failed'}
& "$build/compatibility_test.exe"
if($LASTEXITCODE) {throw 'Compatibility tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/diagnostics_test.cpp" -o "$build/diagnostics_test.exe" -lshell32
if($LASTEXITCODE) {throw 'Diagnostics test compilation failed'}
& "$build/diagnostics_test.exe" "$build/diagnostics-test"
if($LASTEXITCODE) {throw 'Diagnostics tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/text_cache_test.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build/text_cache_test.exe" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Text cache test compilation failed'}
& "$build/text_cache_test.exe"
if($LASTEXITCODE) {throw 'Text cache tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/texture_upload_test.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build/texture_upload_test.exe" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Texture upload test compilation failed'}
& "$build/texture_upload_test.exe"
if($LASTEXITCODE) {throw 'Texture upload tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/cooldown_pulse_test.cpp" -o "$build/cooldown_pulse_test.exe"
if($LASTEXITCODE) {throw 'Cooldown pulse test compilation failed'}
& "$build/cooldown_pulse_test.exe"
if($LASTEXITCODE) {throw 'Cooldown pulse tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/cooldown_adapter_test.cpp" "$root/runtime/hud_bridge.S" -o "$build/cooldown_adapter_test.exe" -lopengl32 -lgdi32
if($LASTEXITCODE) {throw 'Cooldown adapter test compilation failed'}
& "$build/cooldown_adapter_test.exe"
if($LASTEXITCODE) {throw 'Cooldown adapter tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/weapon_lab_test.cpp" "$root/tests/lab_shield_call.S" "$root/runtime/score_bridge.S" -o "$build/weapon_lab_test.exe"
if($LASTEXITCODE) {throw 'Weapon Lab test compilation failed'}
& "$build/weapon_lab_test.exe"
if($LASTEXITCODE) {throw 'Weapon Lab tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/local_recorder_test.cpp" -o "$build/local_recorder_test.exe" -lbcrypt
if($LASTEXITCODE) {throw 'Recorder test compilation failed'}
& "$build/local_recorder_test.exe" (Join-Path $build ('recorder-test-' + [guid]::NewGuid().ToString('N')))
if($LASTEXITCODE) {throw 'Recorder tests failed'}

& $Compiler -O2 -fno-devirtualize -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/quick_chat_test.cpp" "$root/runtime/score_bridge.S" -o "$build/quick_chat_test.exe" -lopengl32 -lole32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Quick chat test compilation failed'}
& "$build/quick_chat_test.exe" (Join-Path $build ('chat-test-' + [guid]::NewGuid().ToString('N')))
if($LASTEXITCODE) {throw 'Quick chat tests failed'}

& $Compiler -O2 -fno-devirtualize -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/auto_messages_test.cpp" "$root/runtime/score_bridge.S" -o "$build/auto_messages_test.exe"
if($LASTEXITCODE) {throw 'Auto messages compilation failed'}
& "$build/auto_messages_test.exe" (Join-Path $build ('auto-test-' + [guid]::NewGuid().ToString('N')))
if($LASTEXITCODE) {throw 'Auto messages tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type "$root/tests/wheel_image_test.cpp" "$root/runtime/score_bridge.S" "$root/runtime/hud_bridge.S" -o "$build/wheel_image_test.exe" -lopengl32 -lgdi32 -lbcrypt -lcomdlg32 "$audioCodec" -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE) {throw 'Wheel image test compilation failed'}
& "$build/wheel_image_test.exe" "$root/tests/fixtures/wheel-icon.png" (Join-Path $build ('wheel-image-' + [guid]::NewGuid().ToString('N')))
if($LASTEXITCODE) {throw 'Wheel image tests failed'}

& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type -fno-devirtualize "$root/tests/source_audit_test.cpp" "$root/runtime/score_bridge.S" -o "$build/source_audit_test.exe" -lbcrypt
if($LASTEXITCODE) {throw 'Source audit compilation failed'}
& "$build/source_audit_test.exe"
if($LASTEXITCODE) {throw 'Source audit tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror -Wno-cast-function-type -fno-devirtualize "$root/tests/combat_stats_test.cpp" "$root/tests/combat_call.S" "$root/runtime/score_bridge.S" -o "$build/combat_stats_test.exe"
if($LASTEXITCODE) {throw 'Combat stats compilation failed'}
& "$build/combat_stats_test.exe"
if($LASTEXITCODE) {throw 'Combat stats tests failed'}
& $Compiler -O2 -std=c++17 -static -Wall -Wextra -Werror "$root/tests/music_module_test.cpp" "$audioCodec" -o "$build/music_module_test.exe" -lbcrypt -lcomdlg32 -lmfreadwrite -lmfplat -lmfuuid -lole32 -lshell32 -lwindowscodecs
if($LASTEXITCODE){throw 'Music tests compilation failed'}
& "$build/music_module_test.exe" (Join-Path $build ("music-test-"+[guid]::NewGuid().ToString("N")))
if($LASTEXITCODE){throw "Music module tests failed"}
