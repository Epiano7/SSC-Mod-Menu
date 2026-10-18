# Completed feature history

## Cosmetics notice and menu routing — October 18

Moved the ownership notice into a standalone update-style popup, with animated rainbow Acid Pass text. Separated its page and button identifiers from Freecam and private-module renaming. Also separated Credits / More people from the Geri challenge toggle. The combined v0.1.19 candidate targets build 26025030. Popup, menu-key, queued-draft, ownership and native-guard checks passed; the popup was visually inspected. Windows/Linux packages are prepared under work/update-26025030-packages. Live gameplay and publication remain pending.


## Camera-relative vehicle controls

Requested October 12, 2026. Status: experimental native adapter implemented for local singleplayer. Available under All Modules > Driving after installing the candidate. Multiplayer keeps native controls until command serialization is reviewed. User confirmed the corrected singleplayer controls work well on October 12.

- Optional toggle; default remains the native car-relative controls.
- W/S/A/D express up/down/left/right relative to the gameplay camera, projected onto the ground plane.
- Interpret the requested 45-degree boundaries as four 90-degree heading sectors, with changes at 45, 135, 225 and 315 degrees relative to the camera. This is four-direction input remapping, not eight-direction steering or snapping the car's physical rotation.
- Translate those intents into the game's normal throttle/reverse/steering inputs using vehicle heading. Keep native acceleration, traction, turning and collision physics.
- Add a small boundary dead band (hysteresis) to prevent rapidly switching controls when a car sits near a sector edge.
- Prototype diagonal inputs, reverse/braking behavior, steering while stationary, rotated cameras and controller support. Keep map/chat/menu controls unaffected and restore original controls immediately on exit or toggle off.
- Decide through playtesting whether discrete remapping feels natural or whether steering toward a camera-relative target direction is preferable. The latter would be a distinct assisted-steering design, not silently added by the initial toggle.
- Confirm the native client input path and server reconciliation before claiming multiplayer compatibility; do not teleport or directly overwrite vehicle rotation/velocity.

Native integration: reviewed build 25847950 call 0x9dd620 invokes control consumer 0x9d4f70 after 0x9dd400 copies seat-zero driver inputs to vehicle offsets 0x14fï¿½0x152. The optional adapter rotates these four booleans, then calls the original consumer. It preserves simultaneous opposite presses, wheel/gear/brake code and all unrelated flags. Full caller and consumer hashes gate attachment. No keyboard injection or position/velocity/heading writes.

Prototype verification: four headings, rotated cameras, normalized analog/diagonal magnitude, five-degree boundary hysteresis, vehicle changes, and disabled/invalid fallbacks passed. Native fixture tests additionally cover driver/passenger identity, local-authority Survival, remote/co-op/dedicated rejection, death, missing bounds, rotated-view fallback, other-car updates preserving hysteresis, and mutations of reviewed executable code. Live singleplayer driving was confirmed by the user; Proton remains untested. Multiplayer command serialization is still pending.

The implemented singleplayer controls are removed from the active todo list. Multiplayer validation and remaining platform checks stay on that list.

## October 17, 2026: color refresh and menu key candidate

Installed to the owner's Steam instance after passing cache/protocol, asynchronous worker, and menu/color regression tests; all 109 native call targets match the installed game. No public release was published.

- Rotating bounded lookup batches prevent full rosters starving chat senders; separate batches preserve unexpired cached styles.
- Killfeed draws retain attribution from the account predicate instead of re-identifying a formatted label.
- Interface settings capture and persist the menu key, with Right Shift default and reset.

Live match color verification remains in the active list.

## October 17: queued draft advisor refresh

Installed candidate after passing queued-draft regression and existing color/keybind tests. The advisor uses the native displayed offer list rather than requiring the actor's separate copy to match, and no longer treats the button input-enabled flag as visibility. Held Ctrl, local actor, remaining skill points, offer validation and transition guards remain. Tests cover consecutive picks without releasing Ctrl, lagging actor offers, disabled input flags, key release, transition, duplicate offers, empty queue and nonlocal context. Live player confirmation remains pending.

## October 17: account-row and vehicle name attribution

- Added canonical account adapters at 0x62719a, 0x6298d8, and 0x640dab; these work without a match roster and apply local or saved shared styles over native green/rainbow. Removed the two misleading leaderboard rank-number hooks.
- Vehicle names now resolve the actual occupant through the native owner record, including bot colors. A further actor name at 0x41adde now uses its RBX owner rather than formatted text.
- Added exact native-body guards and validated all 110 declared hook edges against build 26009609. Warning-as-error runtime/test builds and cache, worker, color/keybind, and queued-draft regressions passed.
- Installed to Steam with backup `work/color-keybind-build/installed-backup-20261017-234026`. No public release. Live visual coverage remains on the planned list.
