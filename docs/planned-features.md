# Requested features

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

Native integration: reviewed build 25847950 call 0x9dd620 invokes control consumer 0x9d4f70 after 0x9dd400 copies seat-zero driver inputs to vehicle offsets 0x14f–0x152. The optional adapter rotates these four booleans, then calls the original consumer. It preserves simultaneous opposite presses, wheel/gear/brake code and all unrelated flags. Full caller and consumer hashes gate attachment. No keyboard injection or position/velocity/heading writes.

Prototype verification: four headings, rotated cameras, normalized analog/diagonal magnitude, five-degree boundary hysteresis, vehicle changes, and disabled/invalid fallbacks passed. Native fixture tests additionally cover driver/passenger identity, local-authority Survival, remote/co-op/dedicated rejection, death, missing bounds, rotated-view fallback, other-car updates preserving hysteresis, and mutations of reviewed executable code. Live singleplayer driving was confirmed by the user; Proton remains untested. Multiplayer command serialization is still pending.
