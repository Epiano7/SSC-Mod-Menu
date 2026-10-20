# Draft research uploads

Draft Advisor has an explicit, persisted **Share round data for research** toggle,
off by default. Once enabled, rounds upload automatically to
`https://api.epiano7.dev/api/draft-training/v1/round`. Private draft access is
required. Ordinary recording can remain off. Disabling sharing discards the
current round and clears pending files; an HTTP request already in flight may
finish. No account ID, handle, chat, machine specifications, paths, authorization
data, wall-clock timestamps, or native pointers are included.

The collector records supported BR round observations only. Random 128-bit IDs
identify individual rounds and make retry delivery idempotent. They do not link
players or multiple rounds into an identity. Numeric class/skill/mutation IDs,
sparse skill levels, candidate scores/evidence, recommended skill, API contribution,
build changes, relative event times, and ten-second numeric stat samples form the
training payload. End-of-round samples and partial-round/truncation flags preserve
coverage limits. Unknown stats are omitted, never supplied as zero. Win/place,
outgoing damage, assists, and causal contribution are not inferred.

## Event schema 1

- `type: 0`: observed build (class, sparse skill levels, mutations).
- `type: 1`: displayed recommendation, candidates, scores, evidence and build.
- `type: 2`: pick **inferred from a single build delta**. `skill`, `kind`,
  `followed`, and `advice_event` are all null when ambiguous. This is not a click
  hook or proof of player intent. A multi-level/multi-skill change is ambiguous.
- `type: 3`: allowlisted numeric observations. No arbitrary string metrics.
- `type: 4`: current/previous native offer IDs and native reroll counter, captured
  independently of the selection key. Counter resets after picks and is not the
  number of remaining rerolls. No offer change is automatically labeled a reroll.

Native offer capture double-reads the snapshot and checks the current executable's
reviewed generator body. Unknown revisions skip this capture. Current build
26047692 generator is RVA `0x921300`, length `0x5a66`, SHA-256
`19f9399f0925c8b78efd0b0ebe6851962ff7b124dc99a42d7bf7af19694962a3`.
The current/previous vector and counter accesses were reviewed at
`0x921436`, `0x92143d`, `0x921518`, and `0x92152e`. The shared draft context
`0xe02e98 + 0x4b8` is the displayed-offer vector `0xe03350` already used by the
advisor. Capture is read-only and never invokes the native generator.

## Limits and storage

Each round has at most 512 events and 256 KiB uncompressed. The render thread
does no file or network I/O and avoids serializing unchanged recommendation
frames. A worker writes compact JSON to `draft-training-pending` under mod state,
with at most 32 pending files (about 8 MiB) and four incoming memory items.
Network failures retry in the background. Permanently rejected payloads are
retained as .rejected files with an HTTP status receipt instead of blocking the queue.
Private-access loss pauses delivery and preserves pending data; explicit opt-out clears it.

Server: `C:\Services\ssc-draft-training`, scheduled task **SSC Draft Training**,
LOCAL SERVICE, loopback port 18744, separate Caddy route. Existing auth/cosmetics
services are untouched. Data is gzip-compressed in `data`, capped at 1 GiB and
100,000 rounds; records older than 90 days are pruned on the next ingestion.
Rate limits: 12 writes/minute/source and 120 total/minute. Proxy source IP is
used transiently in memory for limiting, not stored with rounds. The service
does not log request bodies or identifiers. Normal network infrastructure still
necessarily processes connection addresses.

Anonymous uploads are untrusted research, not verified gameplay or automatic
model updates. Analyze coverage/ambiguity, validate and deduplicate data before
offline training. User skill and selection bias prevent interpreting a few wins
as proof of an advisor's causal benefit.

## Deployment and verification

Initial Caddy configuration was backed up as `Caddyfile.before` in the service
directory. Initial deployment and the later native-event schema update have
separate JSON records there. Synthetic acceptance/idempotency smoke-test data
was moved out of `data` to `synthetic-smoke-test.json.gz`.

Client build/inference/bounds fixture and service strict schema, privacy field
rejection, compression, deduplication, conflict, size, storage and HTTP rate-limit
tests are provided. The recovered October 19 match was accepted and its three canonical payload hashes
verified against server storage. The next live match automatically uploaded three complete rounds; all 27
recommendation records match the ordinary recording. Expanded research context
remains deferred pending a dedicated live test. Uploads require private access and explicit consent.

## Predictor status

The prior test build contains captures and deterministic helper primitives, not
a validated future-offer predictor. The October 19 game update changed draft
weighting branches around old `0x925de2` / current `0x925dad` as well as addresses.
A port of the old routine cannot be assumed correct. Table data alone is not
sufficient: eligibility, class/build/progression, history and reroll transitions
must all agree. The current native generator mutates state and must not be called
on the live game or a shallow copy for speculative previews.

Next predictor work is a pure current-rule implementation, exact visible-offer
reproduction, then one-step reroll shadow validation. Full future sequences are
branches conditional on actual choices and must be recalculated after changes.
No prediction or reroll recommendation is enabled by this telemetry change.
