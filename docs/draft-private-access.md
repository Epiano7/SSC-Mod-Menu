# Draft Advisor private access

Skill and syringe recommendations require the existing signed private-access
grant (`ssc_auth::available()`). The Bot Highlight enabled preference is separate:
disabling Bot Highlight does not revoke Draft Advisor access. No new mandatory
token claim, activation code, or server protocol is introduced.

The entitlement gates settings actions, build preparation, skill scoring/API
submission, syringe advice, recommendation audit events, and final compositing.
Native draw hooks remain attached for later activation but call the original
game UI when locked. Final compositing checks access again so prepared borders
cannot remain visible after expiration or revocation. Already submitted HTTP
requests may finish, but their results cannot be displayed without access.

Existing device/account binding and signed grant expiry/renewal rules still
apply. This is client-side access control, not a claim of tamper resistance.

Validation covers no-service denial, blocked settings toggles, audit suppression,
locked panel pass-through with no build preparation/API submission, and loss of
access between border preparation and rendering. Existing valid-access OpenGL
fixtures cover both skill and automatic syringe highlighting.

Installed on October 16, 2026, for Steam build 25979506. Live gameplay and an
FPS improvement remain unverified. Server authorization task
`01a0ef21-f4da-7581-8149-80d81253a062` was notified to align integration guidance.
