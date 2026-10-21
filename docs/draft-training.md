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

## Limits and storage

Each round has at most 512 events and 256 KiB uncompressed. The render thread
does no file or network I/O and avoids serializing unchanged recommendation
frames. A worker writes compact JSON to `draft-training-pending` under mod state,
with at most 32 pending files (about 8 MiB) and four incoming memory items.
Network failures retry in the background. Permanently rejected payloads are
retained as .rejected files with an HTTP status receipt instead of blocking the queue.
Private-access loss pauses delivery and preserves pending data; explicit opt-out clears it.

Uploaded rounds are compressed on the server and retained for up to 90 days.
Connection addresses are processed transiently for rate limiting, not stored in
round records. Normal network infrastructure still processes connection addresses.

Anonymous uploads are untrusted research, not verified gameplay or automatic
model updates. Analyze coverage/ambiguity, validate and deduplicate data before
offline training. User skill and selection bias prevent interpreting a few wins
as proof of an advisor's causal benefit.
