# Installer

SSC-Mod-Menu-Setup.exe is one compact, borderless Windows application. It discovers the Steam game folder and shows Install or Update according to the ownership manifest. Uninstall is next to the primary action. Repair is always visible beneath Browse; it reports an invalid folder or unsupported game version without changing files.

Repair installs into a clean compatible folder, restores missing or modified owned files, and preserves settings/custom audio. It refuses unowned conflicts and unsafe manifests. Original files and the manifest remain in a uniquely named directory under SSCMods-Recovery beside the mod folder for recovery. Interrupted replacement tests cover every payload boundary.

Owned paths remain opengl32.dll, SSCMods/runtime.dll, SSCMods/Uninstall.exe and SSCMods/install-manifest.json for compatibility with existing installs. The game executable and assets are never overwritten. Uninstall validates ownership and retains modified/unrelated files.

The UI runs without administrator access. Install/Update, Repair, and Uninstall request Windows elevation only when performing an operation. File work runs asynchronously and prevents concurrent actions. The installed uninstaller stages itself outside the game folder before removal.

## In-game updates

The runtime checks once at a verified main menu, or manually through About. It does not automatically open an update prompt during gameplay. A compatibility-only menu survives an unsupported game update; its manual update controls remain available with Right Shift. Automatic checking is deferred when the game state cannot be verified.

The unelevated helper checks the latest stable GitHub release, rejects drafts/prereleases/downgrades, downloads SSC-Mod-Menu-Setup.exe, and verifies the GitHub asset SHA-256 digest and size. It launches the verified installer, requests elevation if needed, and waits for a ready signal before the runtime closes the game. The installer verifies game compatibility and process identity, waits for exit, repairs/updates owned files, then returns to the unelevated helper to launch Steam app 308600. The normal installer window is not shown.

Windows UAC cannot be bypassed. Downloads and cancelled elevation leave the game running. Failed file replacement rolls back; recovery files and the job status remain available. A game that refuses to close cancels the installation after two minutes.

The repository needs a public, non-draft stable release for anonymous updates. No GitHub token is bundled. Release tags must be vMAJOR.MINOR.PATCH (or MAJOR.MINOR.PATCH), and versions must increase. The installer asset must retain its exact filename. Direct download links can avoid a GitHub navigation step.

Protocol source: [GitHub release assets](https://docs.github.com/en/rest/releases/assets).
