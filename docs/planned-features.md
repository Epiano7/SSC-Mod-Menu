# Requested features

## Cosmetics sharing policy: remaining rollout

Client ownership checks and the personalized notice are implemented; the owner confirmed the notice works. Remaining: migrate older clients/server policy, verify real purchase/expiry/account-switch transitions, and reduce initial shared-color lookup delay where measurable. Unknown ownership keeps sharing disabled.


## High priority: consistent shared username colors

Requested October 17, 2026. Status: chat lookup batching, killfeed attribution, and configurable menu key passed regression tests and were installed October 17, 2026. Leaderboard/account-row and vehicle identity fixes also passed regression/native checks and were installed October 17. Live visual verification and any unmapped surface variants remain open.

- Leaderboard colors were confirmed working by the owner. Audit remaining remote profile, friends/recent-player, and spectator widget variants; do not infer full coverage from the leaderboard check.
- Live-check a confirmed sharing friend: canonical account rows now override native green/rainbow, with unknown accounts preserving native appearance (regression passed).
- Review cars, killfeed, winner/results lists, spectator targets, and contextual name labels individually. Replace roster-dependent text matching with attributed account/actor data where available; handle renamed players, duplicate labels, departed players, and stale rows safely.
- Live verification pending: scroll back to Mang's public-chat messages while receiving a confirmed active shared style; verify the installed batching fix refreshes out-of-roster senders. Full-roster asynchronous regression passed.
- Live verification pending: verify both killfeed names use their intended bot/shared colors in a match. Native predicate-to-draw attribution and mixed local/shared/bot regression passed.
- Verify every username surface with solid, rainbow, and custom gradients, including a native-rainbow player and a confirmed sharing friend. Test sharing off, expiry, account changes, and remote players outside the current match.
- Audit evidence and surface checklist: [Username color audit](username-color-audit-20261017.md). The lookup for `jyxalag` returned no active style at the time checked; that alone does not prove their sharing preference or native account key.

## Driving: remaining validation

- Review multiplayer command serialization and server reconciliation before enabling camera-relative controls there.
- Validate controller input and Proton behavior.

The completed singleplayer feature and its implementation notes have moved to [completed feature history](completed-features.md).
