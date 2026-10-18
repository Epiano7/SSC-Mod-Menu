# Username color coverage audit - v0.1.18

Reviewed October 17, 2026 against released commit 92127d08473082cbc83e50a7dafd0e1689da9a57 and Steam build 26009609. This is a source/native-call audit, not a claim that every screen has been visually tested.

## Confirmed leaderboard gap

Public chat supplies its sender account string to `shared_account`, which queues a lookup independently of the match roster. Remote generic leaderboard/profile text instead goes through `shared_text`: it must resolve to an actor in the current world before a shared style can be used. Having the same player's style cached from chat does not remove that requirement. Main-menu account widgets explicitly apply only the local user's override.

Consequently an out-of-roster remote leaderboard/profile name keeps its native color. The user's report of Mang working in public chat but retaining native rainbow in the leaderboard matches this gap. A specific screen's record layout still needs examination before adding an account-aware hook; matching arbitrary display names alone would miscolor duplicate names.

## Surface-by-surface checklist

| Username surface | Current shared-color path | Finding / remaining verification |
| --- | --- | --- |
| Public chat sender, including main-menu chat | Sender account via `ssc_chat_name_draw` | Directly supported; user confirms Mang works. |
| In-match chat sender | Same sender bridge | Directly supported in code; live check needed. |
| Tab scoreboard | Row slot resolves to actor via `ssc_score_draw` | Directly supported; live check needed. |
| Overhead player name | Actor via `overhead` / `draw` | Directly supported for active actors; live check needed. |
| Actor-attached HUD/name labels | Actor via `ssc_actor_name_draw` | Directly supported for active actors. Exact screen variants need visual check. |
| Showdown participant list | Dedicated actor bridge, including inactive/eliminated actors | Directly supported; live winner/final-result variants must be checked separately. |
| Round modifier chooser's name | Dedicated modifier actor bridge | Directly supported; live check needed. |
| Vehicle occupant text | Generic complete-label to current actor mapping | Conditional: needs an unambiguous current actor/label. No separate vehicle identity bridge. |
| Killfeed / compact event names | Event predicate and generic label mapping | Conditional: needs retained, unambiguous actor attribution. Cached names after actor removal may fall back. |
| Other compact player/result lists near minimap | Generic or showdown-specific path depending on widget | Cannot declare every winner/result variant covered without mapping the exact widget. |
| Ranked/global leaderboard remote rows | Generic text fallback | Confirmed missing reliable out-of-roster account attribution. |
| Remote profile/account cards | Generic text; local widget adapter is local-only | Same missing remote attribution; individual card variants require mapping. |
| Friends/recent-player/cached account lists | No dedicated remote identity adapter identified | Same roster-dependent fallback; screen-specific verification pending. |
| Own account header / own profile and list labels | Local account comparison and local widget scope | Local override supported; does not prove remote sharing support. |
| Spectator target / other contextual player notifications | Actor-attached or generic text, depending on native widget | Needs exact call attribution and visual verification; not yet certified. |
| Message bodies, typed chat input, quick-wheel message content | Deliberately excluded | Correct: text mentioning a name is not a username label. |

## Native-call verification

All 109 declared adapter sites still call their expected targets in build 26009609. The shared UTF-16 render functions have 70 direct call edges: 67 hooked and three deliberate message/input exclusions. Narrow render functions have many more callers for arbitrary UI text; these are not all username surfaces, and their conversion into the hooked wide renderer does not supply missing identity attribution.

Evidence: workspace `work/name-coverage-20261017/native-edges.json` and `audit.py`.

## Additional limitations

- Lookups request up to 32 accounts. Current roster entries are inserted before observed chat senders, so a full roster can starve other visible accounts.
- Lookups run roughly every 15 seconds; newly observed accounts may briefly display native colors.
- Shared profiles are leased for 120 seconds; client cached appearances expire within 30 seconds. Offline or opted-out players eventually fall back to native colors by design.
- Duplicate display names and stale/reused actor slots deliberately refuse ambiguous attribution.

## Fix direction

Capture the stable account key from each leaderboard/profile/list row and pass it through a scoped name-render adapter. Include visible UI accounts in bounded asynchronous lookup scheduling without roster starvation. Keep display text, sorting, layout, native alpha and unrelated text unchanged. Add regression fixtures for out-of-match leaderboard rows, renamed/duplicate names, stale rows, cache expiry and more than 32 visible accounts. Then visually check each surface with solid, rainbow and custom-gradient sharing, including native-rainbow players such as Mang.

## Friend-name green follow-up

Public cosmetics lookup for the requested key `jyxalag` returned `ok:true, profiles:[]` during the October 17 check. This establishes no active shared style under that key at that moment; it does not establish the player's sharing preference or prove that the displayed name equals their native account key.

There is no friend-green exemption in the current shared tint adapters. If an attributed sender/actor resolves to a live shared style, the style replaces the incoming native RGB/phase. If no shared style resolves, native friend green remains. Out-of-roster friends/leaderboard labels can still retain green because of the missing attribution described above, even when another surface has a cached shared style. Live verification with a confirmed publishing friend remains outstanding.

## Subsequent fix: October 17, 23:40 local

This section supersedes the old gap and lease descriptions above. The server now persists last published colors until explicitly removed; Mang's published gradient was confirmed present. Client lookups rotate bounded batches and observe off-roster accounts.

Native tracing found the old hooks at 0x6249c4 / 0x63e2a7 rendered rank numbers, not account names. The actual canonical-name calls are 0x62719a (R14+0x38), 0x640dab (RSI+0x38), plus account-list call 0x6298d8 (RDI+0x38). Those now use the account directly, independently of world actors, for both own and shared colors. Native friend green and rainbow are overridden only when a style exists.

Vehicle call 0x9e35d3 now carries RSI and validates occupant at owner+0x368 -> +0x60 before applying actor/local/bot/shared appearance. Actor-label call 0x41adde now carries RBX. Killfeed continues using the earlier raw-account predicate-to-draw fix.

Regression fixtures exercise absent-world account rows, own/shared/unknown accounts, native green and rainbow input, renamed vehicle labels, bot/local/remote/empty occupants, alpha preservation, and scope cleanup. All 110 native call targets and five new body guards matched. These are not live screenshots: profile/friend/recent/spectator variants outside these verified paths still require individual visual confirmation. Evidence: `work/name-surfaces-20261018/validation.json` and disassembly files.
