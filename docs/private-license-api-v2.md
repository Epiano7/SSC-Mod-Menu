# Bot activation-code and device-key API — v2

Implemented September 30, 2026 at the owner's request. This is the current bot-access method. It **does not verify ownership of a Steam account or game license**. The server checks an owner-managed SteamID allowlist, a one-use activation code bound to that SteamID, and proof of possession of an enrolled device key. Knowing a SteamID alone is insufficient. Someone with a stolen code can claim its associated SteamID and enroll first; distribute codes privately.

Ordinary beta access is separate and closed. No beta code, opaque beta token, beta JWT, or feature named `bot_module` authorizes this protocol.

## Deployment and readiness

Base/issuer: `https://sscmmauth.epiano7.dev`. Game/AppID claim: numeric `308600`.

| Method | Path | Purpose |
|---|---|---|
| GET | `/v2/license/status` | Readiness of the code/device-key flow |
| GET | `/v1/keys` | Existing public Ed25519 signing keys, also used for license JWTs |
| POST | `/v2/license/challenge` | One-use challenge for activation or renewal |
| POST | `/v2/license/activate` | Redeem a code and enroll the device key |
| POST | `/v2/license/renew` | Fresh startup/renewal grant using an enrolled key |

`GET /v2/license/status` returns:

```json
{"ok":true,"api_version":2,"app_id":308600,"authentication":"activation_code_device_key","steam_identity_verified":false,"permission":"bot-highlighting","max_token_ttl_seconds":120}
```

**Do not gate this flow on `/readyz`.** That endpoint belongs to the original Steam-ticket flow and still returns 503 `not_configured` without publisher credentials. Its `/v1/challenge` and `/v1/authorize` contract and issuer are preserved; they are not silently downgraded. The new protocol has a distinct JWT audience and authentication-method claim.

Use verified HTTPS, refuse redirects, and never log bodies, codes, tokens, proofs, or private keys. POST requires exact `Content-Type: application/json`, Content-Length, no compression/chunking, and at most 8192 bytes. Unknown fields are rejected. Base64url is unpadded throughout.

## Client state and first activation

1. Read the current game account's SteamID64 as a string. This is a local consistency check, not trusted Steam identity proof.
2. Create a durable Ed25519 device key pair, distinct from all beta/Steam-ticket keys. Persist the private key securely **before activation** so a lost response can be recovered. On Windows use DPAPI scoped to the current user. Under Proton use a tested secure-storage strategy and restrictive filesystem permissions; do not assume Wine/Proton DPAPI provides the same protection as Windows. Never use a shared key embedded in the mod, a hardware fingerprint, or the game publisher's key.
3. Provide one generic, explicit "Activate private access" action where the owner can paste the code once. Do not add ordinary-beta enrollment UI or display the bot feature itself before authorization. Do not repeatedly prompt. Subsequent launches should renew silently.

POST `/v2/license/challenge`:

```json
{"steam_id":"<current-local-SteamID64>","device_public_key":"<base64url of raw 32-byte Ed25519 public key>","purpose":"activate"}
```

Success (example timestamps, not a usable challenge):

```json
{"ok":true,"api_version":2,"challenge":"<32-character nonce>","purpose":"activate","expires_at":"2026-09-30T12:01:00.000Z","app_id":308600,"authentication":"activation_code_device_key","steam_identity_verified":false}
```

Challenge expires after 60 seconds and is bound to the supplied SteamID, key, and purpose. A challenge response does not indicate approval or reserve a code. Up to 1024 outstanding challenges are permitted.

Activation codes are case-sensitive strings `SSCB-` followed by 43 base64url characters (256 random bits). Remove accidental surrounding whitespace in the UI, then hash the exact resulting code; never lowercase it. Sign this exact UTF-8 message with the device private key:

```text
ssc-bot-license-v2\nactivate\n<challenge>\n<base64url(SHA256(UTF-8(code)))>
```

Each `\n` means one LF byte; no final newline. POST `/v2/license/activate`:

```json
{"challenge":"<nonce>","code":"SSCB-<43 characters>","proof":"<base64url of the 64-byte Ed25519 signature>"}
```

The server consumes the challenge, validates the signature and allowlist, atomically consumes the valid code, and enrolls the key. Codes default to 24-hour redemption expiry and may be issued for at most seven days. Code expiry limits enrollment time, not the lifetime of an already enrolled device. A code cannot enroll a second device. Up to five device records per account are allowed. A disabled device cannot re-enroll the same public key unless the owner explicitly re-enables/removes its record.

## Silent startup and renewal

On every startup, with a stored key, POST the same challenge request but `purpose:"renew"`. Sign:

```text
ssc-bot-license-v2\nrenew\n<challenge>\n-
```

The final field is a literal ASCII hyphen; no final newline. POST `/v2/license/renew`:

```json
{"challenge":"<nonce>","proof":"<base64url signature>"}
```

No code or bearer token is sent for renewal. The server checks the signature, current account approval/expiration, and enrolled device enabled flag. All failed/uncertain requests require a fresh challenge. If activation's response is lost, retain the saved key and try a new renewal challenge; activation may already have enrolled it. Do not immediately generate a replacement key or automatically spend another code.

Both activation and renewal return:

```json
{"ok":true,"api_version":2,"authorization":"<compact EdDSA JWT>","expires_at":"2026-09-30T12:02:00.000Z","device_id":"<43-character JWK thumbprint>","authentication":"activation_code_device_key","steam_identity_verified":false}
```

## Required grant validation

Use a maintained Ed25519/JWT implementation. Obtain trusted public keys only from the fixed HTTPS `/v1/keys` URL. Require header `alg=EdDSA`, `typ=JWT`, matching `kid`, JWK `kty=OKP`, `crv=Ed25519`. Never trust token-supplied keys/URLs or unsigned response fields.

Verify signature and exact claims:

| Claim | Required value/check |
|---|---|
| `iss` | `https://sscmmauth.epiano7.dev` |
| `aud` | `ssc-bot-license:308600` (not the original `ssc-mod:308600`) |
| `appid` | numeric `308600` |
| `sub` | string equal to the current local SteamID64 |
| `permission` | `bot-highlighting` |
| `auth_method` | `activation_code_device_key` |
| `steam_identity_verified` | boolean `false`; do not represent this as verified Steam login |
| `challenge` | exactly the current pending challenge; clear pending state on acceptance |
| `iat`, `nbf`, `exp` | integer epoch seconds, `nbf == iat <= now < exp`, `0 < exp-iat <= 120` |
| `jti` | 22-character base64url grant ID |
| `cnf.jkt` | thumbprint of this device's stored public key |

Thumbprint is unpadded base64url(SHA256(UTF-8 of the following exact whitespace-free JSON in this key order)):

```json
{"crv":"Ed25519","kty":"OKP","x":"<device_public_key>"}
```

Renew around 60 seconds with jitter. A grant lasts no more than 120 seconds and is shortened by account approval expiry. Enforce wall-clock expiry and a monotonic deadline so clock rollback/suspend cannot prolong access. No offline startup, token persistence, or grace after expiry. During an outage a currently valid lease may remain only until its existing deadline. Account/device revocation blocks new grants immediately; an issued offline lease may remain for at most 120 seconds.

Hide and disable bot behavior, menu/search entries, hotkeys, commands, and saved-setting restoration until valid authorization. Clear active grants on account changes, logout, expiry, or shutdown; ignore stale async responses. Keep the stored device key separate per account/profile or require reactivation when switching identities. Never move a bot grant into the beta entitlement store.

## Errors and limits

Application errors: `{"ok":false,"api_version":2,"code":"<stable code>","error":"<human message>"}`.

- 400: `malformed`.
- 401: `invalid_challenge` (expired/used/wrong purpose), `invalid_proof`.
- 403: `denied` (account), `activation_denied` (code/account/device binding), `device_denied` (unknown/disabled device), `device_limit`.
- 413: `body_size`.
- 429: `rate_limited`, `busy`.
- 503: `not_configured`, `unavailable`.
- Unknown routes/methods: 404; Caddy may return an empty/non-JSON error body.

The bot service shares its 120 requests/minute global limit across v1 and v2, with 8 in-flight handlers, 32 connections, 8KiB headers/body, and 10-second inbound timeouts. Back off with jitter (5, 10, 20, max60 seconds), never per-frame. No public admin interface exists. Server logs include only event/time/status; client bot authorization must not appear in logs or telemetry.
