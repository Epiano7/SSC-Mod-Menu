# Private access

The current client uses the [v2 activation-code/device-key protocol](private-license-api-v2.md) at https://sscmmauth.epiano7.dev. This proves possession of an owner-issued code and enrolled device key, not Steam identity or game ownership. The local game SteamID is an account consistency check. Ordinary beta is closed, even if an old beta token exists

Open the module settings sidebar and select **Private access**. Paste the code and select **Activate**. There is no startup popup. The bot module stays hidden and disabled until the signature, session, account, device, permission and expiry checks succeed. Successful activation enables the module and clears the code. Later launches silently renew with the saved key

Windows saves a separate per-account device seed in the existing state directory as `private-device-<SteamID64>.bin`, encrypted with current-user DPAPI. A saved key is read back before any activation request. A missing key is created only for an explicit activation; corrupt/unreadable keys are never silently replaced. Keys remain after network errors, denial and revocation. Uncertain activation is recovered by renewal using that same key. No tokens are persisted and no codes, keys, requests, responses or authorization activity are logged

Grants last at most 120 seconds, renew after 55–65 seconds, and use both wall-clock and suspend-inclusive monotonic expiry. Existing grants cannot extend past their original deadline during an outage. All behavior uses the same Access state. Account changes, logout, suspend/resume and shutdown clear access; stale asynchronous results cannot install a grant. Networking runs on a worker with bounded requests, normal TLS verification, no redirects and no shared client secret

The original v1 protocol primitives are retained for regression coverage; production does not call Steam-ticket authorization or gate v2 readiness on `/readyz`

## Validation and limits

Native Windows isolated tests cover exact proofs, signed claims, malformed/proxy errors, approval/denial, activation and silent startup renewal, lost response recovery, replay, expiry, revocation, account changes, stale responses, clocks, DPAPI round-trip and per-account isolation. Menu/hotkey tests cover the accessible activation form and hidden-module bypass protection. Tests use isolated mocks, never production approval fixtures

Proton private activation is currently blocked with a secure-storage-unavailable message. Wine DPAPI is not assumed to match native Windows protection. A validated native secret-store bridge and restrictive Linux filesystem permissions are still required before enabling this feature under Proton. Public modules remain available; this limitation applies only to private authorization

Live owner activation and gameplay rendering are separate manual checks. No test result should be described as verified Steam authentication
