# Shared cosmetics

## Using it

Open **All Modules > Cosmetics > Shared Cosmetics**. Turn on **Share my colors**
to publish the existing solid color, rainbow or two/three-color gradient. Turn on
**See other players' colors** to receive them. Both controls default off and save
across restarts. Cosmetics must be enabled to publish. No code, enrollment,
sharing ID, administrator approval or device key is required.

Other users need a compatible mod and receiving enabled. Unmodified clients keep
the game's appearance. Remote appearance uses attributed actor, scoreboard and
chat-sender account keys, including main-menu chat. Unattributed cached remote
leaderboard/profile labels retain their original appearance. Local overrides
continue to cover the existing local-name paths.

## Trust and lifetime

Account keys are routing labels, not verified identities. The server deliberately
accepts public cosmetic updates without ownership validation; a modified client
can submit another account's colors. These updates grant no private/beta access,
carry no trusted gameplay statistics, and cannot change names, commands or modules.
Only supported color modes and bounded integer colors are accepted.

HTTPS/TLS validation, redirect refusal, body limits, per-source/global rate limits
and client stale-response checks remain enabled. Public styles live in bounded
server memory for at most 120 seconds. Clients renew after approximately 60 seconds
and cache received styles for at most 30 seconds, bounded by the server lease.
Wall-clock and monotonic expiry reject rollback/suspend extensions. Opt-out sends
remove; offline leases expire naturally. Restart never resurrects old profiles.
Networking remains asynchronous and each request has a 10-second watchdog.

## Protocol

Base: `https://api.epiano7.dev/api/cosmetics/v1`.
Canonical compact JSON, sorted object keys, no duplicate keys. Requests <=4096
bytes; replies <=16384 bytes. POST Content-Type is exactly application/json.

- GET /status: {ok:true,api_version:1,app_id:308600,service:"shared-cosmetics",lease_seconds:120}.
- POST /publish: {account_key:"native_account",operation:{action:"publish",style:{mode:"solid",colors:[5623039]}}}.
  Remove uses action:"remove",style:null. Response: {ok:true,account_key:"native_account",lease_seconds:120} (0 for remove).
- POST /lookup: {accounts:["native_account"]}, at most 32 distinct account keys.
  Response: {ok:true,profiles:[{account_key:"native_account",style:...,ttl_seconds:1..30}]}.

Modes: solid has one color, rainbow none, gradient two or three; integer colors
0..16777215. Account keys are 1..96 lowercase ASCII letters/digits or `_@.-`.
Unknown fields, privilege claims and the old challenge/proof protocol are rejected.
No tokens or keys are read, written or uploaded by this sharing revision.

## Platform validation

Windows publication and isolated client/service tests passed. Two-player visual checks and Proton gameplay verification remain outstanding. Linux support is experimental.
