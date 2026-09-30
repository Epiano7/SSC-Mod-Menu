# SSC authorization API v1

Base/issuer: `https://sscmmauth.epiano7.dev` (no trailing slash).
TLS certificate validation is mandatory. Use normal OS trust on Windows and an appropriate trusted CA store under Proton; never disable TLS verification. Redirects must not forward tickets. Request bodies and grants must not appear in client logs or telemetry.

No cookies, passwords, client-shared secret, or client-claimed SteamID field. All requests/responses use JSON; POST requires exact `Content-Type: application/json`, Content-Length, no compression/chunking, and body <=8192 bytes. Unknown fields on protocol POSTs are rejected. No CORS browser integration is intended.

## Routes

| Method and path | Result |
|---|---|
| GET `/healthz` | 200 `{"status":"ok"}`; process liveness only |
| GET `/readyz` | 503 `{"status":"not_configured"}`, or 200 `{"status":"configured_unverified"}` |
| GET `/v1/keys` | 200 `{"keys":[JWK]}`; public Ed25519 signing key only |
| POST `/v1/challenge` | 200 challenge; requires server Steam configuration |
| POST `/v1/authorize` | 200 signed grant after Valve verification and allowlist check |

Everything else returns 404. No administration, account enumeration, or private label endpoints exist. Responses are `Cache-Control: no-store`. Errors normally use `{"error":"CODE"}`; Caddy-generated errors may have empty/non-JSON bodies, so clients must check HTTP status before parsing success.

## Challenge

Generate a fresh ephemeral Ed25519 key pair for this exchange. Keep the private key only in process memory. Send its raw 32-byte public key as unpadded base64url:

```json
{"client_public_key":"<43-character unpadded base64url>"}
```

Example response shape (placeholders, not usable credentials):

```json
{
  "challenge":"<32-character unpadded base64url>",
  "identity":"ssc-bot-v1:<same challenge>",
  "expires_at":1800000060,
  "app_id":123456
}
```

Challenge is valid for 60 seconds. Require `app_id` to equal the game's expected AppID. Call `SteamUser()->GetAuthTicketForWebApi(identity.c_str())` and wait for the matching `GetTicketForWebApiResponse_t` with `m_eResult == k_EResultOK`. Hex-encode exactly `m_cubTicket` bytes of `m_rgubTicket`. Maximum ticket size here is 2560 bytes. Do not use `GetAuthSessionTicket`, fabricate tickets, or send a local SteamID as authentication.

## Proof and exchange

Compute SHA-256 of the **binary** ticket, then unpadded base64url-encode the digest. Sign this exact UTF-8/ASCII message using the ephemeral Ed25519 private key (LF separators, no final newline):

```text
ssc-auth-v1\n<challenge>\n<base64url(SHA256(binary_ticket))>
```

The displayed `\n` means byte 0x0A, not a literal backslash and letter n. Encode the 64-byte signature as unpadded base64url (86 characters).

POST `/v1/authorize`:

```json
{"challenge":"<challenge>","ticket":"<hex ticket>","proof":"<base64url signature>"}
```

The challenge is consumed atomically before Steam verification. This remains consumed on Steam outage, rejection, or dropped response. On any retry, obtain a new challenge, new key pair, and fresh Steam ticket. The server supplies AppID/key/identity to Valve itself; these are not client-controlled request fields. Only Valve's `steamid` is used for allowlist membership; `ownersteamid` is not used.

Success:

```json
{"authorization":"<compact EdDSA JWT>","expires_at":1800000120}
```

Once the exchange completes, cancel that Steam ticket handle with `CancelAuthTicket`; retain the authorization/session state only until expiry. Cancel outstanding tickets on failures/shutdown too.

## Token verification

Header is `{"alg":"EdDSA","typ":"JWT","kid":"<key id>"}`. Use a maintained Ed25519/JWT implementation available to the mod's language. Fix the allowed algorithm to EdDSA and curve to Ed25519. Obtain trusted keys only from the fixed HTTPS `/v1/keys` endpoint (or an explicitly maintained pin); never follow URLs or keys supplied inside the token. Validate the JWS signature over the ASCII `base64url(header).base64url(payload)` bytes before using claims.

Required claims:

| Claim | Exact check |
|---|---|
| `iss` | `https://sscmmauth.epiano7.dev` |
| `aud` | `ssc-mod:<actual decimal game AppID>` |
| `appid` | numeric expected AppID |
| `sub` | canonical SteamID64 **string**, equal to current local Steam user |
| `permission` | `bot-highlighting` |
| `challenge` | this pending exchange's challenge; consume pending state on acceptance |
| `iat`, `nbf`, `exp` | integer epoch seconds, `nbf == iat <= now < exp`, `0 < exp-iat <= 120` |
| `jti` | random grant ID (22-character base64url); do not reuse an accepted exchange |
| `cnf.jkt` | SHA-256 JWK thumbprint of this exchange's ephemeral public key |

Thumbprint input is this exact UTF-8 JSON with lexicographic field order and no whitespace:

```json
{"crv":"Ed25519","kty":"OKP","x":"<client_public_key>"}
```

`cnf.jkt` is unpadded base64url(SHA256(input)). `expires_at` outside the JWT is a convenience; only the signed claims authorize access. Reject wrong account, audience, issuer, app, permission, session, nonce, algorithm, signature, key, or time. Example checks and signing bytes are implemented in `examples/client.mjs`. It is a Node reference, not a Steam SDK adapter or drop-in mod patch.

## Module behavior and renewal

Default to hidden/disabled. After full verification, enable both UI and behavior through one central authorization state. Check that state in hotkeys, commands, rendering hooks and saved-setting restoration. Existing public modules must continue normally when private authorization fails.

Renew around 60 seconds with a little jitter, using a fresh exchange. Keep a still-valid lease during temporary renewal errors, but never extend its expiry. Enforce expiry using both validated wall-clock time and a monotonic deadline captured during the exchange, so clock rollback and suspend/resume cannot extend access. Recheck on account change, reconnect, wake, and each use of the private feature. On startup, expiry, account change, or logout, hide the module and clear private state. Never persist grants for offline startup.

No Steam/network availability at startup means no private module. During an outage, access ends at the existing expiry, no later than 120 seconds after issuance. Disabling an allowlist account prevents subsequent grants; the same maximum residual lease applies to revocation. This does not prevent someone patching local code or extracting included module functionality.

## Errors and limits

| Status | Codes / meaning |
|---|---|
| 400 | `malformed` |
| 401 | `invalid_challenge`, `invalid_proof`, `invalid_ticket` |
| 403 | `denied` (unknown, disabled, expired, or removed; no distinction) |
| 404 | unrecognized method/path |
| 413 | `body_size`, or proxy body-size rejection |
| 429 | `rate_limited` or `busy` |
| 503 | `not_configured`, `steam_unavailable`, `unavailable` |

Do not assume error bodies always contain JSON. Back off with jitter (e.g. 5, 10, 20, up to 60 seconds), obey expiry, and avoid retry loops on every frame. Current global allowance is 120 HTTP requests per minute for the small group, so cache public keys in memory and refresh on renewal/unknown `kid` as needed. The server cannot distinguish an official mod from a custom client merely from its port or User-Agent; genuine Steam verification and the allowlist determine authorization.
