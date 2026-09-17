# Building

Requires Windows x64, MinGW-w64 with C++17, Python with `pefile`, and the Windows .NET Framework C# compiler. The runtime uses native Win32/OpenGL; no game assets are included.

```powershell
./scripts/Build-Runtime.ps1 -Compiler 'C:\path\to\g++.exe' -Python 'C:\path\to\python.exe'
./scripts/Build-Installer.ps1 -RuntimeDirectory './build/runtime'
```

Build-Runtime runs runtime tests. Build-Installer runs installer engine, UI workflow and updater process-handoff tests. Close Skillshot City first: the running-game guard also applies to installation fixtures. Test outputs stay under build/.

Packaging requires the exact validated runtime/proxy hashes recorded in Build-Installer.ps1. A fresh compile may produce different PE hashes; validate the resulting binaries, update the approved hashes before packaging. Do not bypass the checks. Without RuntimeDirectory, the installer is a payload-free preview.

Native font/palette checks can additionally run `menu_ui_test.exe OUTPUT_DIRECTORY GAME_DIRECTORY GAME_EXE`. They read your local game installation.


## Native compatibility

`runtime/compatibility_data.h` contains short function locators, normalized full-function fingerprints, and bindings for the validated native layout. The runtime resolves them once before installing hooks. A missing/ambiguous locator, changed function body, disagreeing data references, or invalid target section pauses the dependent modules. This is conservative compatibility checking, not a guarantee that arbitrary future game ABIs are compatible.

After validating a new executable's call sites, calling conventions and object layouts, regenerate the manifest with Python packages `pefile` and `capstone`:

```powershell
python scripts/generate_compatibility.py 'C:\path\to\SkillshotCity.exe'
./build/runtime/compatibility_test.exe 'C:\path\to\SkillshotCity.exe'
```

The synthetic resolver tests cover moved code, changed bodies, ambiguous/missing locators, module isolation and invalid data targets. `diagnostics_test` exercises report generation without crashing the game. Installer revision checks remain strict: startup tolerance for an already installed mod does not authorize installing into an unvalidated executable.

Crash diagnostics are best effort. Forced termination, fail-fast errors, or another component replacing the exception filter may prevent a report. The mod preserves the previous exception handler and does not attempt to resume a corrupted process.
