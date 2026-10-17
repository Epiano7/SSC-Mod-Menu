# Native compatibility review — Steam build 25979506

Game SHA256: `4061ca998403c589dfddd6b0cbbd3df6e66af6e1177286a57951b0813835238a`.
Baseline: build 25954344, SHA256 `8af24e050ed67c4026eeea92071dbd2aeada0b5ea105fed9d3cc87797e8c1de6`.

The previous guards rejected six module groups (mask 0xAF). The update relocates native functions and changes some bodies. No checks were disabled to restore support.

## Review evidence

- Reviewed 184 corresponding functions and 242 existing bindings. The regenerated manifest contains 156 dependency functions and 279 binding checks (246 addresses).
- All 112 direct intercepted calls still target their corresponding native functions. Their preceding 15 argument-setup instructions match after relocation normalization. Freecam's 22 additional imported/direct calls retain instruction and target identity.
- The missing 1d0630 locator was a relocated switch table. The larger 432ef0 and 22f200 bodies include compiler stack-layout changes; intercepted call argument windows remain unchanged. Conflicting inferred constant mappings were excluded; binding-site evidence resolves consumed addresses.
- World update fields near +be20 shifted by four bytes. Runtime consumers do not directly use these fields. The guarded camera timing span remains equivalent. The mod continues to call native update functions for their changed behavior.
- The 1aa0f0 change is an integer argument (8 to 12) passed into native string construction/formatting; the Freecam wrapper retains the original call and ABI. No mod-side structure field depends on this constant.
- Acquired skill, mutation and draft routines retain their consumed field layout and hook arguments. Differences in their guarded bodies are relocated switch-table addresses/data. All six acquired-build guards and three advisor-specific guards were updated after comparison.
- The leaf weapon helper retains its complete 152-byte instruction sequence after relocation normalization; referenced float constants match. The round-clock helper was uniquely located using its complete masked 276-byte body.
- All 33 FNV guard spans first matched the baseline executable. Camera caller bytes and private cosmetic SHA256 guards were separately ported. No unknown runtime address consumers remain; unregistered compatibility resolve keys: zero.

## Verification

Warnings-as-errors builds; compatibility probe reports all groups available (255/255). A mutated guarded instruction is still rejected (253/255, needs_review). Freecam, Driving, creator crosshair and acquired-build tests include changed-code rejection. Recording tests cover late joins, round grouping and the native HUD clock. The native clock fixture now copies the complete 276-byte guarded helper (it previously copied only 247).

Menu tests cover settings, disabled compatibility groups, draft audit deduplication, UI renders and log fallback. The main logger now uses shared append access and emits a process-specific fallback log with the Windows error if the main log cannot be written. The old log's stale timestamp alone does not establish its cause.

Live gameplay, FPS, remote color sharing, and live API recommendations still require a restarted game session. Offline/native tests do not establish those outcomes. No release has been published and no worker baseline was changed in this development patch.
