# Combat tracking status

Development status, October 2, 2026. This is not a claim of complete online combat coverage

## Implemented foundation

- Shared round observations run when either Auto Messages or Local Round Recording is enabled; recording alone never dispatches messages
- Local actor changes reset round baselines and projectile counters
- Interrupted observation and render gaps longer than two seconds invalidate projectile totals rather than reporting a complete round
- Recording schema 2 includes typed metrics and availability reasons, including a final tracking snapshot in `recording_stopped`
- Recording stays optional and local; no account identifiers or native pointers are exported
- `round_end_observed` distinguishes an observed round end from an interrupted recording; `full_round_timing` applies only to timing, not to every metric
- Missing values are JSON null, distinct from an observed zero

## Metric coverage

| Values | Current source / limitation |
| --- | --- |
| Current level, HP, shield, inventory, weapon properties, money, syringes | Validated native state reads; sampled values, not combat events |
| Round time, time alive/dead, levels gained | Countdown baseline plus observations; unavailable when coverage is insufficient |
| Round deaths, damage taken | Native cumulative counters with a round baseline; damage taken does not identify the attacker or ability |
| Shots/projectiles fired | Validated local primary-weapon spawn hook; requires round-start coverage |
| bulletHits, playerHits, npcHits, impactDamage | Unavailable: client contact hook misses remotely simulated actors; nominal impact is not applied damage |
| Damage dealt, healing done/received, shield restoration | No validated authoritative event source yet |
| Kills, assists, reloads, skill/throwable uses, accuracy | No validated complete source yet; do not infer from deaths, ammo changes or partial contacts |
| Per-ability damage/healing breakdown | Blocked on event coverage and source attribution |

Existing unavailable Auto Message variables remain hidden and saved rules remain preserved. Recording availability labels do not unlock those variables

## Current binary evidence

Inspected executable SHA256: `2b9776be60e32e5df38c5b7f1b3c0128bee2d88b3dcf2c64d7845105149e60c5`

The current actor-update decoder at RVA `0x894ea0` writes the existing local death (`+0x954`), level (`+0x870`) and damage-taken (`+0xd14`) fields. The previously investigated special-NPC damage field (`+0xc1c`) is absent from that decoder. This does not prove that other network messages cannot provide outgoing events; those paths still need investigation

## Required event contract for the breakdown

An authoritative applied event needs a match/round identity, stable event ID, owning actor, target, originating ability/weapon, and actual HP/shield delta. Keep damage, healing and shield restoration separate. Exclude overheal and overkill; preserve ownership for delayed damage, summons and DOT. A missing source must remain Unknown, and missing event coverage must remain partial

Do not derive healing by summing positive HP changes: respawns, max-health changes, prediction corrections and simultaneous damage/healing make that inaccurate. Do not sum modifier bonuses and final applied damage as separate contributions

## Verification still required

Synthetic tests cover lifecycle, typed serialization, unavailable values, actor changes, interrupted recording and final snapshot retention. Native Windows online rounds and Proton gameplay must separately validate the candidate. No new outgoing damage/healing hook has been approved by these tests
