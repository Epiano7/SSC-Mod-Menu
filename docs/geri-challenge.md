# Geri Challenge mode

Open **All Modules > Misc > Geri Challenge** to hide the skill-draft and syringe selection panel. The setting is off by default and is saved locally. Turn it off to restore the normal panel; Disable All also turns it off.

This is a presentation aid for the fan-made challenge. It does not enforce or certify a match without skills or stims. External shortcuts and automatic selections have not been verified as blocked.

## Compatibility and scope

The adapter suppresses the dedicated skill-panel call, including its own selection, reroll and syringe UI handlers. A separate hook makes the world-click dispatcher ignore the hidden skill-panel hitbox, while retaining its inventory and other UI checks. It does not modify actor data, inventory, skills, points or syringe counts. Currency and other HUD routines remain separate.

A separate compatibility group validates the caller and panel dependencies. An unsupported game build leaves this option unavailable rather than applying an unchecked hook.

## Validation

Automated checks cover disabled forwarding, enabled suppression, independent world-click hitbox bypass, preservation of other UI checks, restoration, settings persistence, Disable All, unavailable-build handling and menu layout. The current Windows executable passes dependency fingerprint checks. The Windows tester confirmed that the panel hides and Ctrl-pings work through its former area. Proton gameplay is not yet verified.

Before release, test in a match: enable before a draft appears, confirm the panel stays hidden after leveling, check movement, shooting and unrelated HUD/menu interactions, then disable and confirm normal selection returns. Repeat across death, round and match transitions. Check keyboard shortcuts and automatic selection separately; UI suppression alone is not rule enforcement.
