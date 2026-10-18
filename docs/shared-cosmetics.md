# Shared cosmetics

## Using it

Open **All Modules > Cosmetics > Shared Cosmetics**. Turn on **Share my colors**
to publish the existing solid color, rainbow or two/three-color gradient. Turn on
**See other players' colors** to receive them. Both controls default on for fresh settings and save
across restarts. Existing saved choices are preserved. Cosmetics must be enabled to publish. No code, enrollment,
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
and client stale-response checks remain enabled. As of October 17, 2026, the server
saves the last published appearance to its runtime directory and retains it across
player disconnects and service restarts. Explicit sharing opt-out removes the saved
appearance. Updating a color replaces the saved appearance atomically before the
server acknowledges success. Unchanged client renewals do not rewrite storage.

Clients still cache lookup results for at most 30 seconds and refresh asynchronously;
only clients with receiving enabled apply them. The v1 `lease_seconds:120` response
is retained for compatibility with existing clients, not as a server retention limit.
Previously expired appearances cannot be recovered; those players must publish once
again. The store is limited to 10,000 accounts and rejects new entries when full
rather than silently evicting saved colors. Invalid storage fails visibly at startup.
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
