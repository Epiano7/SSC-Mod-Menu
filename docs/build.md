# Building

Requires Windows x64, MinGW-w64 with C++17, Python with `pefile`, and the Windows .NET Framework C# compiler. The runtime uses native Win32/OpenGL; no game assets are included.

```powershell
./scripts/Build-Runtime.ps1 -Compiler 'C:\path\to\g++.exe' -Python 'C:\path\to\python.exe'
./scripts/Build-Installer.ps1 -RuntimeDirectory './build/runtime'
```

Build-Runtime runs runtime tests. Build-Installer runs installer engine, UI workflow and updater process-handoff tests. Close Skillshot City first: the running-game guard also applies to installation fixtures. Test outputs stay under build/.

Packaging requires the exact validated runtime/proxy hashes recorded in Build-Installer.ps1. A fresh compile may produce different PE hashes; validate the resulting binaries, update the approved hashes before packaging. Do not bypass the checks. Without RuntimeDirectory, the installer is a payload-free preview.

Native font/palette checks can additionally run `menu_ui_test.exe OUTPUT_DIRECTORY GAME_DIRECTORY GAME_EXE`. They read your local game installation.

`text_cache_test.exe GAME_DIRECTORY` compares cached text against the reference rasterizer using the installed fonts, including scaling, clipping and animated colors. `texture_upload_test.exe` verifies allocation, updates and resizing on a hidden OpenGL window.

`cooldown_adapter_test.exe GAME_EXE` checks the executable's compatibility fingerprints, then maps it without starting the game to exercise the native readiness function against synthetic player/world records. The regular test also exercises the assembly bridge and verifies that only the selected icon's opacity argument changes. The pulse state-machine test covers completion, recasting, stale observations, invalid inputs and disabled settings.


## Native compatibility

`weapon_lab_test.exe GAME_EXE` checks bounded catalog reads and calculation edge cases, then compares the displayed-DPS formula against the fingerprinted native helper on synthetic records. Weapon Lab reads base definitions only when opened or refreshed. Its simple firing/reload model excludes bursts and added damage effects; it is not a simulation of an equipped build or real combat.

`runtime/compatibility_data.h` contains short function locators, normalized full-function fingerprints, and bindings for the validated native layout. The runtime resolves them once before installing hooks. A missing/ambiguous locator, changed function body, disagreeing data references, or invalid target section pauses the dependent modules. This is conservative compatibility checking, not a guarantee that arbitrary future game ABIs are compatible.

After validating a new executable's call sites, calling conventions and object layouts, regenerate the manifest with Python packages `pefile` and `capstone`:

```powershell
python scripts/generate_compatibility.py 'C:\path\to\SkillshotCity.exe'
./build/runtime/compatibility_test.exe 'C:\path\to\SkillshotCity.exe'
```

The synthetic resolver tests cover moved code, changed bodies, ambiguous/missing locators, module isolation and invalid data targets. `diagnostics_test` exercises report generation without crashing the game. Installer revision checks remain strict: startup tolerance for an already installed mod does not authorize installing into an unvalidated executable.

Crash diagnostics are best effort. Forced termination, fail-fast errors, or another component replacing the exception filter may prevent a report. The mod preserves the previous exception handler and does not attempt to resume a corrupted process.

Weapon Lab includes editable target health/shield, manual damage/reload modifiers, up to four comparison columns and timed duel playback. `weapon_lab_test.exe GAME_EXE` also compares 20,000 synthetic health/shield cases with an isolated arithmetic block in a private mapped game image; the game is not started. No live actor or player data is used. The shield balance values are read from the running game only after compatibility checks. Calculator results currently exclude unsupported multiple-impact weapons, bursts, melee/explosives and damage-over-time weapons; Minigun has a three-impact estimate. Perks, range falloff, travel, passive recovery and defense effects are not modeled. Timing remains an estimate, rather than a verified reproduction of the firing loop.

## Linux / Proton packaging

The native Linux installer packages the Windows mod payload for Steam Proton; it does not support the native Linux game executable

Build the launcher with Zig using `scripts/Build-Linux-Launcher.ps1`, then package both platforms using `scripts/Build-Release.ps1`. Required parameters are `RuntimeDirectory`, `Python`, `BuildDirectory`, `LinuxPythonArchive`, and `LinuxLauncher`

The Linux Python archive must be the pinned CPython 3.13.15 x86-64 glibc install-only archive from [python-build-standalone 20260924](https://github.com/astral-sh/python-build-standalone/releases/tag/20260924). `Build-Linux.py` verifies its SHA256 and the approved runtime payload hashes. Keep bundled dependency notices intact

Publish `SSC-Mod-Menu-Setup.exe` and `SSC-Mod-Menu-Linux.tar.gz` as separate release assets. Linux remains experimental: successful installation does not establish that every module or the update/restart flow works under Proton. See [Linux instructions](../linux/README.md) for the current validation scope

Private authorization adds a pinned, statically linked libsodium dependency and WinHTTP. See [authorization implementation and validation](private-authorization.md) for build prerequisites and live-test limitations.
