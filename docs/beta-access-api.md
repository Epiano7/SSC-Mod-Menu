# Beta enrollment API

Status: deployed API; readiness confirmed September 30, 2026. Live code redemption and Proton checks remain manual validation

Base URL: `https://api.epiano7.dev`
AppID: `308600`

This API is for normal beta enrollment only. It uses a client-reported SteamID64 and does not cryptographically prove Steam ownership. It must never confer permission for the bot module. Bot highlighting uses the independent Steam-ticket protocol in [private-authorization-api.md](private-authorization-api.md)

## Current client

Ordinary beta is closed. The legacy contract below is retained for reference and protocol regression tests, not an enabled UI or startup flow. No beta credential grants private access

## Requests

`GET /api/mod-auth/status`

Success: HTTP 200, `{"ok":true,"service":"mod-entitlements","api_version":1,"app_id":308600}`

`POST /api/mod-auth/redeem`, Content-Type `application/json`

```json
{
  "app_id": 308600,
  "steam_id": "<local SteamID64>",
  "code": "<trimmed single-use beta code>",
  "client_version": "0.1.8-beta",
  "platform": "windows"
}
```

Legacy code support remains internal for protocol regression tests; the production menu cannot submit codes. Codes are never bundled, logged or saved

`POST /api/mod-auth/check`, Content-Type `application/json`, Authorization `Bearer <opaque token>`

```json
{
  "app_id": 308600,
  "steam_id": "<current local SteamID64>",
  "client_version": "0.1.8-beta",
  "platform": "windows"
}
```

`windows` is currently used for this PE client, including under Wine. A future `proton` value requires a reliable project signal, not process-name detection

## Success responses

Redemption: HTTP 200

```json
{
  "ok": true,
  "api_version": 1,
  "app_id": 308600,
  "access_token": "<opaque printable ASCII bearer token, max 8192 characters>",
  "entitlement": {
    "grant_id": "<nonempty ID, max 256 characters>",
    "steam_id": "<same canonical SteamID64 string>",
    "features": ["beta"],
    "expires_at": null,
    "permanent": true
  }
}
```

Check: same `api_version`, `app_id` and `entitlement`, with `"authorized":true` instead of `ok`, and no access token. Only the exact `beta` feature grants normal beta enrollment. `bot_module`, `bot-highlighting` and all other feature names in these responses confer **no bot access**

For temporary entitlements, use `permanent:false` and an ISO-8601 UTC timestamp with `Z` or `+00:00` (optionally fractional seconds). Expired records cannot authorize. Permanent records require a null expiry

After redeem, the client saves the token before making a separate check. Successful redemption alone does not activate features. The consumed code is cleared even if the subsequent check fails. A lost token after an interrupted redemption needs a replacement code; retry must not silently spend that code again

## Errors and client behavior

Errors use `{"error":"<human-readable message>","code":"<stable_code>"}`. Only the `code` field selects the client state; human-readable messages are never interpreted as error codes. Non-2xx responses remain fail-closed

| Error | Client action |
|---|---|
| code_unavailable | Invalid/unavailable code; no grant |
| invalid_token | Clear token and return to enrollment |
| steam_id_mismatch | Disable for this account; keep account binding |
| expired / revoked | Disable; show reason |
| rate_limited | Ask user to wait |
| invalid_app_id / invalid_steam_id / invalid_content_type / invalid_request | Integration error; no grant |
| internal_error / unreachable / malformed | Fail closed; offer retry |

The client checks exact account/app/version types, required entitlement structure and requested feature. Responses are bounded at 16 KiB and reject duplicate JSON keys. Networking is asynchronous, concurrent requests are suppressed, and each HTTP operation has a 10-second deadline and shutdown cancellation. TLS verification stays enabled and redirects are refused

Tokens are stored with current-user DPAPI in the existing mod state directory as `beta-access.bin`. No plaintext fallback. A successful online check is required on each process startup. No persisted authorization and no offline fallback. Account changes invalidate pending responses and cached grants

## Readiness and separation

The client checks the exact status service, API version and AppID before redemption or a saved-token check. No token or code is sent if readiness fails. The same asynchronous worker handles startup readiness, saved-token validation and user requests

Normal beta enrollment currently unlocks no gameplay features; it is available for future beta distribution. Signed bot authorization remains exclusively on `https://sscmmauth.epiano7.dev/v1/...` with the exact existing JWT issuer. The beta host must never be substituted for the signed bot host or vice versa
