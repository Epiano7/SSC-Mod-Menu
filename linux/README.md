# SSC Mod Menu — Linux / Proton (experimental)

**Installation has been confirmed on Arch Linux with regular Steam and Proton Experimental. Most mod features are still untested on Linux. Other distributions, Flatpak Steam, and the in-game updater have not been verified.**

The installer runs natively on Linux, but the game and mod run through Steam Proton. The native Linux build of Skillshot City is not supported by this package.

## Install

1. In your Steam Library, right-click **Skillshot City → Properties → Compatibility**
2. Enable **Force the use of a specific Steam Play compatibility tool** and select **Proton Experimental**
3. Wait for Steam to download the Windows build, launch the game once, then close it
4. Fully exit Steam using **Steam → Exit**
5. Download and extract **SSC-Mod-Menu-Linux.tar.gz**
6. Open **SSC-Mod-Menu-Setup** inside the extracted folder
7. Confirm the game folder and select **Install / Update**
8. Open Steam, launch Skillshot City, and press **Right Shift**

Changing Steam's global default compatibility tool alone may leave the native Linux game installed. The selected game folder must contain **SkillshotCity.exe**. Use **Properties → Installed Files → Browse** in Steam to locate it.

Python and the GUI libraries are bundled. No separate Wine or Python installation, terminal commands, or manual launch-option edits are needed. Keep the whole extracted folder together. If your file manager asks which application should open the installer, enable **Allow executing as a program** in the setup file's Properties, then open it again.

The package targets x86-64 Linux with glibc and X11, or a Wayland desktop with XWayland. Compatibility with every distribution is not guaranteed.

## Updates and removal

Use **Install / Update**, **Repair**, or **Uninstall** in the installer. Settings, recordings, and custom sounds are preserved. First installation and uninstallation require Steam to be closed while its launch setting is changed. Existing launch options are preserved. Updates to a managed installation require the game to be closed.

The in-game **Update and Restart** handoff is implemented but untested on Proton. Use a freshly downloaded installer if it does not work.

The installer keeps its launch helper under `~/.local/share/ssc-mod-menu` (or `XDG_DATA_HOME`), so removing the downloaded folder does not break launching. Keep or redownload the installer for repair or removal. Uninstall restores the previous launch option while preserving unrelated edits.

## Troubleshooting and logs

Open the installer and select **Export Logs** to save a ZIP for a bug report. Include your distribution, Proton version, and what happened. Review the ZIP before sharing it; nothing is uploaded automatically.

The export includes installer/startup logs, available Proton and mod runtime logs, and basic system/build information. It excludes Steam account settings, custom sounds, and round recordings. Home paths are masked where possible, but other paths, hardware details, or game information may remain. Each exported log is limited to its last 4 MiB.

If the installer cannot open, check `~/.local/state/ssc-mod-menu/logs` (or `XDG_STATE_HOME/ssc-mod-menu/logs`). The `setup-startup.log` file records launcher/Python startup failures. Proton diagnostics are enabled, with the previous session retained.

Audio import, HUD editing, cosmetics, quick chat, Auto Messages, clipboard integration, Discord presence, and update/restart behavior still need broader gameplay testing on Linux.

Bundled dependency notices are included under `licenses` and `python`.
