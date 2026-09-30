# SSC Mod Menu

An unofficial client-side mod for Skillshot City, with a Windows installer and an experimental Linux installer for Steam Proton.

## Download

[Download SSC Mod Menu for Windows](https://github.com/Epiano7/SSC-Mod-Menu/releases/latest/download/SSC-Mod-Menu-Setup.exe)

Run the installer and select your Skillshot City Steam folder. After installation, launch through Steam and press **Right Shift** to open the mod menu.

## Linux / Proton (experimental)

[Download SSC Mod Menu for Linux](https://github.com/Epiano7/SSC-Mod-Menu/releases/latest/download/SSC-Mod-Menu-Linux.tar.gz)

**Installation has been confirmed on Arch Linux with regular Steam and Proton Experimental. Most mod features are still untested on Linux.** Other distributions, Flatpak Steam, and in-game updating have not been verified

In **Skillshot City → Properties → Compatibility**, enable **Force the use of a specific Steam Play compatibility tool** and choose **Proton Experimental**. Let Steam download the Windows build and launch it once, then close the game and fully exit Steam

Extract the download, open **SSC-Mod-Menu-Setup**, select the folder containing **SkillshotCity.exe**, and click **Install / Update**. Reopen Steam and launch the game

The installer runs natively; the game and mod use Proton. The native Linux game is not supported yet. No separate Wine setup is needed. If the setup file does not open, allow executing it as a program in its file Properties

[Full Linux instructions and log export](linux/README.md)

## Modules

- **Sound Replacer:** import WAV or MP3 replacements for sound effects and music, preview clips, adjust individual volume, and replace related variants together. **All Music Tracks** uses one custom song across the music playlist. Track lengths are shown, and replacements apply after restarting the game
- **Cosmetics:** customize your name with animated rainbow colors or a solid color, visible only to you
- **Discord Presence:** show game status, available round and level details, elapsed time, and ranked SSC rating. No developer setup or token is required. Linux connects to the native Discord app, with support for regular and Flatpak socket locations
- **HUD Editor:** move and resize eight HUD groups, save layouts, and reset individual elements. Team-roster positioning is currently unavailable
- **Custom Quick Chat:** customize the quick-react wheel with up to 12 messages per slot. The first message labels the wheel, and each use sends a randomly chosen message from that slot's list. Rearrange slots, choose icons or import images, and preview the in-round and round-ended layouts
- **Auto Messages:** create optional Battle Royale message rules using events, conditions, variables, and repeat counts. Enable or rename rules, reference the variable guide, and share rules through compact import codes. Both `{variable}` and `[variable]` are supported

Enable Auto Messages before a round starts for complete supported round counters. Online hit counts and impact-damage totals are currently unavailable. Saved rules using unavailable variables display a warning and skip sending rather than report incomplete values

## Other features

- **Weapon Lab:** search weapons by name, tier, or type; compare up to four weapons with stronger values highlighted; adjust health, shields, and miss chance; and watch a timed duel with reloads. Simulations are estimates and do not account for skills or every special effect
- **Local Round Recording:** optionally save Battle Royale round data on your PC, including class, level, health, and weapon inventory snapshots. Recording is disabled at each launch, only runs during rounds, and uploads nothing
- **Interface:** adjust UI scale, choose which modules appear in the quick menu, and customize animation and HUD display preferences. Text edits save when leaving the field
- **Updates:** check for releases through the in-game prompt. Available updates can be dismissed for that version, and release notes can be disabled separately

For version-by-version changes, see the [GitHub releases](https://github.com/Epiano7/SSC-Mod-Menu/releases)

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

## Private access

Privately approved beta testers can enter an activation code under **Private access**. Later launches check access automatically. Public modules do not require a code. Private activation currently requires native Windows; see the [Linux guide](linux/README.md) for platform limits
