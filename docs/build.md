# Building

Requires Windows x64, MinGW-w64 with C++17, Python with `pefile`, and the Windows .NET Framework C# compiler. The runtime uses native Win32/OpenGL; no game assets are included.

```powershell
./scripts/Build-Runtime.ps1 -Compiler 'C:\path\to\g++.exe' -Python 'C:\path\to\python.exe'
./scripts/Build-Installer.ps1 -RuntimeDirectory './build/runtime'
```

Build-Runtime runs runtime tests. Build-Installer runs installer engine, UI workflow and updater process-handoff tests. Close Skillshot City first: the running-game guard also applies to installation fixtures. Test outputs stay under build/.

Packaging requires the exact validated runtime/proxy hashes recorded in Build-Installer.ps1. A fresh compile may produce different PE hashes; validate the resulting binaries, update the approved hashes before packaging. Do not bypass the checks. Without RuntimeDirectory, the installer is a payload-free preview.

Native font/palette checks can additionally run `menu_ui_test.exe OUTPUT_DIRECTORY GAME_DIRECTORY GAME_EXE`. They read your local game installation.
