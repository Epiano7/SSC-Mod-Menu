# SSC Mod Menu

An unofficial Windows client-side mod for Skillshot City.

## Development status

The previous beta has been withdrawn while the next version is prepared. There is currently no supported downloadable release. The development runtime targets the September 15 game update. Unsupported versions retain a compatibility/update menu while native modules stay inactive.

Future downloads will have a direct installer link here. Launch through Steam and use **Right Shift** when running a supported build.

## Modules

- **Sound Replacer:** import WAV or MP3 replacements, match volume and restore originals. Sample rate and mono/stereo conversion are automatic. Restart to apply. Identical sounds share one replacement; import lists the affected names and asks before proceeding. MP3 decoding uses Windows Media Foundation.
- **Cosmetics:** local animated rainbow-name styling using the native palette and verified local account identity.
- **Discord Presence:** game status, available round/level details, elapsed time and ranked SSC rating. Uses the bundled Discord application; no user token is needed.
- **HUD Editor:** reposition and resize nine HUD groups, with saved layouts, resets and 25-600% scaling. Groups include minimap, version/FPS, timer, round/players, money/syringes, weapons/ammo, health/skills, team roster and event feed.

## Beta compatibility

The withdrawn **0.1.0-beta.1** targeted one Windows x64 executable revision. Game updates may require a new mod build..

The HUD editor is experimental: automated geometry, input and fade-coordinate tests passed, but live dragging/resizing and relocated hover-fade behavior still need visual verification. In-app updates check once at the verified main menu. A newer stable release offers **Update and restart**. The downloader verifies the installer before requesting game exit; installation runs without the normal setup window. Windows elevation may still be required.

The compact installer offers Install/Update, Uninstall, and a small Repair link beneath Browse. Repair installs missing files or restores an owned installation while preserving preferences and recovery copies. The game executable and assets stay unchanged. Uninstall preserves modified files and saved mod preferences.


[Build instructions](docs/build.md) | [Contributing](CONTRIBUTING.md) | [Installer details](installer/README.md)
