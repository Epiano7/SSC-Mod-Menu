 <a href="https://epiano7.dev" target="_blank" rel="noopener noreferrer"><img src="https://epiano7.dev/assets/buttons/epiano7.png" width="88" height="31" alt="epiano7.dev"></a>
[![Downloads](https://raw.githubusercontent.com/Epiano7/SSC-Mod-Menu/traffic-data/downloads.svg)](https://github.com/Epiano7/SSC-Mod-Menu/releases)
![Traffic](https://raw.githubusercontent.com/Epiano7/SSC-Mod-Menu/traffic-data/badge.svg)
# SSC Mod Menu

An unofficial client-side mod for Skillshot City, with a Windows installer and an experimental Linux installer for Steam Proton.

Join my discord [here](https://discord.gg/T5h5GUwEG7) and ping me for any help!
## Download

[Download SSC Mod Menu for Windows](https://github.com/Epiano7/SSC-Mod-Menu/releases/latest/download/SSC-Mod-Menu-Setup.exe)

Run the installer and select your Skillshot City Steam folder. After installation, launch through Steam and press **Right Shift** to open the mod menu.

## Linux / Proton (experimental)

[Download SSC Mod Menu for Linux](https://github.com/Epiano7/SSC-Mod-Menu/releases/latest/download/SSC-Mod-Menu-Linux.tar.gz)

**Installation has been tested on Arch Linux with regular Steam and Proton Experimental. Some mod features are still untested on Linux.** Other distributions, Flatpak Steam, and in-game updating have not been verified

In **Skillshot City → Properties → Compatibility**, enable **Force the use of a specific Steam Play compatibility tool** and choose **Proton Experimental**. Let Steam download the Windows build and launch it once, then close the game and fully exit Steam

Extract the download, open **SSC-Mod-Menu-Setup**, select the folder containing **SkillshotCity.exe**, and click **Install / Update**. Reopen Steam and launch the game

The installer runs natively; the game and mod use Proton. The native Linux game is not supported yet. No separate Wine setup is needed. If the setup file does not open, allow executing it as a program in its file Properties

[Full Linux instructions and log export](linux/README.md)

## Modules

- **Sound Replacer:** import WAV or MP3 replacements for sound effects and music, preview clips, adjust individual volume, and replace related variants together. **All Music Tracks** uses one custom song across the music playlist. Track lengths are shown, and replacements apply only after restarting the game
- **Cosmetics:** customize your name with a solid color, animated rainbow, or a custom gradient with up to three colors. Colors are client-side by default and can be shared to other mod menu users if you have bought the DLC or you currently have the Acid Pass active in SSC. [Details and supported displays](docs/shared-cosmetics.md)
- **Discord Presence:** shows game status, available round and level details, elapsed time, and SSC rating. Only works on the discord app (windows and linux) as the web one does not support a custom rpc.
- **HUD Editor:** move and resize most HUD elements, save layouts, and reset individual elements
- **Custom Quick Chat:** customize the quick-react wheel with up to 12 messages per slot. The first message labels the wheel, and each use sends a randomly chosen message from that slot's list. Importing images is supported as well
- **Driving:** available in **All Modules**, with an optional Quick Menu pin. Camera-relative controls remap your movement keys at 45-degree heading boundaries while retaining native steering and braking. With the car facing left, A drives forward, D reverses, and W/S steer. Off by default. Currently singleplayer-only.
- **Freecam:** a separate module that adds a freecam to both singleplayer and multiplayer rounds. Currently it is only available in multiplayer if you are in spectator mode. You can choose between a 2D pan mode (keeps the camera facing the game and allows you to adjust the X and Z on one plane) or the 3D camera (all 3 axis supported with an optional setting to turn down the render distance). There are lots of other small QoL settings under the module menu as well that are not listed.
- **Auto Messages:** message rules using events, conditions, variables, and repeat counts. Choose **Conditions are met** to watch your conditions continuously, or use a round/level/health event. Under **Edit rule → Timing / Cancellation**, require conditions to stay true for up to 60 seconds, delay sending for up to 60 seconds, and optionally cancel if you die. Each rule triggers at most once per round, and pending messages cancel on round changes, disconnects, or rule edits. Existing rules retain their previous timing. Rule sharing uses `SSC-AM3` (a custom sharing system), older versions of the menu used `SSC-AM1` and `SSC-AM2` which are still supported.

- **Match Summary:** experimental post-match scoring, build history, comparisons and Level/XP graphs. Enable Recording before the match; press **F7** after the final round or open the last summary from the module. Scores are calculated on your PC.
- **Flashbang:** choose an inverted black flash or a fully opaque custom image, with the original strength and fade timing.
- **Map Capture Submissions:** optional F9 still and panorama contributions from single-player 3D Freecam. Get an upload code from [Skillshot Guesser](https://ssg.epiano7.dev/contribute), paste it in Location Capture settings and enable sending. Submissions require review before inclusion.

## Other features

- **Weapon Lab:** search weapons by name, tier, or type. Compare up to four weapons with stronger values highlighted; adjust health, shields, and miss chance; and watch a timed duel with reloads. Simulations are estimates and do not account for skills or other round perks like shop purchases 
- **Match Recording:** optionally save observed Battle Royale rounds together in a local match file, including class, level, XP, cash, health, weapons, selected skills, skill levels and any mutations if applicable.
- **Personal Statistics:** browse local recordings by class and date, view summary statistics, and inspect larger level, XP and cash graphs with grid options and hover values. Late joins and unavailable fields are distinguished from complete observations.
- **Interface:** adjust UI scale, choose which modules appear in the quick menu, and customize animation and HUD display preferences.
- **Geri Challenge:** hide the skill-draft and syringe selection panel from **All Modules > Misc**. Disabled by default; switch it off to restore the panel. This hides the UI and does not enforce challenge rules. [Details and testing limits](docs/geri-challenge.md)
- **Updates:** check for releases through the in-game prompt. Once every game boot, the mod will automatically check and prompt the user to update if any are avaliable

For version-by-version changes, see the [GitHub releases](https://github.com/Epiano7/SSC-Mod-Menu/releases)

## Screenshots (wip)

A custom HUD layout in game:

![Gameplay with a custom HUD layout](docs/screenshots/custom-hud.png)

<details>
<summary>Cosmetics and Discord Presence settings</summary>

![Cosmetics settings](docs/screenshots/cosmetics.png)

![Discord Presence settings](docs/screenshots/discord-presence.png)

</details>

## Compatibility

The menu will install into your Skillshot City Steam folder. Each native module checks its dependencies at startup for compatibility issues. Game updates might break some modules depending on the update, in which a hotfix will be released. Larger game changes may still require a mod update.

If something goes wrong, open **All Modules > About > Open logs**, and dm them to Epiano7 or make an issue here with them included. Startup checks are recorded in `runtime.log`; crash reports are saved under `diagnostics` when Windows allows the crash handler to run. These reports are not automatically uploaded to any servers and stay local. Minidumps can contain process data, so review them before sharing.

## Credits
- bencelot (dev of SSC) for giving feedback before this released and for making the game!
- everyone in the [SSC discord](https://discord.gg/skillshotcity) that provided feedback regarding early module dev, thank you!
- for some more specific names:
  - yari_check for testing out the Linux build and helping me suffer through an Arch VM install
  - jyxalag for reviewing and testing early builds
  - alsamandal for resisting against the Manga Recruit monopoly!
  - ironmonkey808 for giving great feedback and making a really cool [SSC Starter Guide!](https://www.youtube.com/watch?v=2uc2Ax52kMk), and for putting up with me 
  joining his lobbies in the last 2 minutes and winning
  - chill for being a chill guy (and for giving some great suggestions)
  - geri, founder of the Geri Challenge (fan-made challenge where you aren't allowed to draft skills)
  - mang for answering a ton of questions I had as a new player and for their work on a synergy chart
  - deus for building my hatred for snipers so much that I always keep one in my second weapon slot :D


[Build instructions](docs/build.md) | [Contributing](CONTRIBUTING.md) | [Installer details](installer/README.md)
