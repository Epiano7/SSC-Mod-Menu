# Confirmed cosmetics-sharing policy — October 18, 2026

Owner confirmed the developer-approved change. Local cosmetic customization remains unrestricted.

| Detected entitlement | Shared appearance |
| --- | --- |
| No qualifying DLC or active pass | None; local appearance only |
| Any owned non-Acid-Pass DLC | Configured solid color; gradients remain local |
| Native, unmodified game grants active rainbow name | Solid, rainbow, or custom gradient |
| Ownership/pass status not loaded | Checking; never claim no purchases or grant sharing |

An expired Acid Pass alone does not grant solid sharing. If another qualifying DLC is owned, expiry falls back to the configured solid color. Receipt of other players' permitted colors remains available regardless of purchases.

## Update notice

Show once per account for this policy revision after ownership loads, with a checking state while unavailable. Keep an accessible explanation in Cosmetics settings. List actual detected DLC titles and active Acid Pass; join up to two with 'and', use a list above two. List local and sharing benefits separately. Show purchase/restart guidance only when not all qualifying DLCs and the active pass are present. Never call unknown ownership 'no DLCs'. Do not repeatedly interrupt gameplay.

## Implementation status

Implemented in this candidate: Steam ownership checks for Starter Pack and Deluxe Edition, observation of the original native rainbow result, local-only fallback when verification is unavailable, solid-color fallback without an active pass, and withdrawal of the client's previously saved shared style when sharing is disallowed. The notice is a standalone popup with animated rainbow Acid Pass text, per-account acknowledgement, and a Sharing Benefits button in Cosmetics.

Current validation targets Steam build 26025030. Ownership tiers, popup rendering, original-result isolation, protocol/cache, worker, menu keybind and queued draft tests passed. The owner previously confirmed live ownership detection and the notice on build 26009609. Live gameplay on 26025030 remains pending.

Enforcement uses game/Steam evidence in the client; there is no new entitlement API. The existing v1 service and older clients have not been migrated, so this must not be presented as server-enforced entitlement verification. Purchase/expiry/account-switch live checks and remaining name-display variants remain in planned-features.md.
