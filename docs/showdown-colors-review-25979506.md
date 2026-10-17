# Showdown colors: Steam build 25979506

Reviewed executable SHA256:
`4061ca998403c589dfddd6b0cbbd3df6e66af6e1177286a57951b0813835238a`.

The participant list in function RVA `0x3ba550..0x3bd63d` formats each actor
through `0x7da750` at `0x3bb13b`, retaining the actor in RDI. The actor is derived
from the participant slot times `0x3478`, plus world actor-array base `+0xa618`.
The formatter output is inserted into the native label; optional native status
and score suffixes are appended. Call `0x3bb428` draws the label via `0x481000`.
RDI remains the actor across that call; subsequent code draws the other row
elements. This name-only call is now routed through a dedicated RDI bridge.

The bridge preserves the existing draw ABI and delegates once to the existing
actor-color adapter logic. Showdown identification allows an inactive actor,
because eliminated participants remain listed, but still validates actor-array
bounds, slot, kind and account. It never derives identity by stripping suffixes.
Other actor paths retain the original active requirement.

The entire containing function is guarded with SHA256
`6bd365ae908e96214904e8de2fa7473e2113d392aeafdc3d9f16d7efa18dc71c`.
Compatibility generation now includes its reviewed call binding. Existing
fingerprints were not weakened. Direct-call audit: 121 calls.

Settings migration enables both sharing settings once when
`shared_defaults_applied` is absent. It persists the marker so later opt-outs
remain effective. Tests cover old off settings, migration, and later opt-outs.

Validation: synthetic shared-color bridge fixture, menu/OpenGL/settings tests,
121 native call target checks, and all 255 compatibility groups pass.
Installed with private draft gating on October 16, 2026. Live showdown visual
verification and an FPS improvement remain unverified.
