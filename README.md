# SSC Mod Menu

An unofficial Windows client-side mod for Skillshot City.

## Download

[Download SSC Mod Menu for Windows](https://github.com/Epiano7/SSC-Mod-Menu/releases/latest/download/SSC-Mod-Menu-Setup.exe)

Run the installer and select your Skillshot City Steam folder. After installation, launch through Steam and press **Right Shift** to open the mod menu.

## Current Official Modules

- **Sound Replacer:** import WAV or MP3 replacements, match volume and restore originals. Sample rate and mono/stereo conversion are automatic. Requires a restart to apply. MP3 decoding uses Windows Media Foundation.
- **Cosmetics:** Customize your name with animated rainbow colors or a solid color, visible only to you.
- **Discord Presence:** game status, available round/level details, elapsed time and ranked SSC rating. No developer setup or token is required.
- **HUD Editor:** reposition and resize nine HUD groups, with saved layouts, resets and 25-600% scaling. Groups include minimap, version/FPS, round clock, round/players, money/syringes, weapons/ammo, healthbar/survival hearts/skills, team roster and event feed.

## Hotfix 0.1.5

- Fixed Space closing text fields
- Expanded related sound groups, including engine variants and kill-combo sounds
- Disabled misleading online hit and impact-damage totals until complete tracking is available

## New in 0.1.4

- **Custom Quick Chat:** customize wheel messages, reorder slots, choose icons or import images, and preview both wheel layouts
- **Auto Messages:** create optional BR rules with conditions, message variables, repeat counts, and shareable codes
- **Sound Replacer:** preview custom sounds, adjust replacement volume, and apply one replacement to related clips, including engine and kill-combo sounds
- **Interface:** choose quick-menu modules and edit text with a cursor, selection, and automatic saving when leaving quick-chat or rule fields

Enable Auto Messages before the round starts for complete supported round counters. Online hit counts and impact-damage totals are currently unavailable because their source does not cover all online hits. Rules using those variables skip sending rather than report incomplete values.

## Screenshots

A custom HUD layout in game:

![Gameplay with a repositioned minimap, centered round timer, and custom HUD layout](docs/screenshots/custom-hud.png)

<details>
<summary>Cosmetics and Discord Presence settings</summary>

![Cosmetics settings with rainbow and solid name-color options](docs/screenshots/cosmetics.png)

![Discord Presence settings with game details, elapsed time and ranked-rating options](docs/screenshots/discord-presence.png)

</details>

## Compatibility

Installs into your Skillshot City Steam folder. Each native module checks its dependencies at startup. Updates that leave those dependencies unchanged can keep working; changed or ambiguous code pauses the affected module instead of using outdated addresses. Larger game changes may still require a mod update.

If something goes wrong, open **All Modules > About > Open logs**. Startup checks are recorded in `runtime.log`; crash reports are saved under `diagnostics` when Windows allows the crash handler to run. Reports stay on your computer and are not uploaded automatically. Minidumps can contain process data, so review them before sharing.

## Credits
- bencelot (dev of SSC) for giving feedback before this released and for making the game ofc
- everyone in the SSC discord that provided feedback regarding early module dev, thank you!

[Build instructions](docs/build.md) | [Contributing](CONTRIBUTING.md) | [Installer details](installer/README.md)
