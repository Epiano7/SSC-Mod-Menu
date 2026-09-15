# Installer

SSC-Mod-Menu-Setup.exe embeds the validated runtime, OpenGL proxy and uninstaller. It discovers Steam or accepts a selected game folder. Install / Update and Uninstall are available from the same window.

Owned paths: opengl32.dll, SSCMods/runtime.dll, SSCMods/Uninstall.exe and SSCMods/install-manifest.json. Updates verify existing ownership and hashes; failed replacements roll back. Uninstall validates the entire manifest before removing files and retains changed files and preferences.

The installer requests administrator access for protected game folders. Inspection and file operations run asynchronously; controls prevent concurrent operations. The installed uninstaller stages a temporary helper so its original executable can be removed. The engine and UI workflows are tested; the elevated helper relaunch still needs end-to-end testing.
