# Native compatibility review — Steam build 26009609

Game SHA256: `9359247b2d000954ff0d2fbe8230b8f65146883e7b27ee4748481f47823b020e`.
Baseline: released v0.1.16, commit `973d772c22e27d94d8b5f1b85378295f1b3cc14a`, game build 25979506.

## Changes and review

The player record grew from 0x3478 to 0x3488 bytes. The world player-vector pointers moved from +0xa618/+0xa620 to +0xa628/+0xa630. Consumed actor fields below the changed tail remain at their original offsets. All direct player iteration and bounds checks use the reviewed new stride and pointers.

184 corresponding function spans and the existing binding sites were compared. 114 intercepted direct calls retain their native callee identities; the six changed argument windows were checked for register/stack changes. The modifier-name bridge still receives the actor in RBX; the showdown bridge still receives its actor in RDI. 22 additional Freecam direct/import call sites retain their callee/import identities. Conflicting inferred constant mappings were excluded rather than used as address evidence.

Changes outside those interfaces include native branch/threshold changes, string-format arguments, compiler register allocation, vector division arithmetic, and relocated jump tables. The mod leaves the native behavior in place. Reviewed decompilation and disassembly confirm the affected player-vector consumers and HUD name-call context. No single global offset delta was applied to structures.

All 33 feature-specific FNV spans were checked against the old executable before updating their addresses, sizes and hashes. Camera caller bytes, bot-spawn and cosmetic rendering SHA256 guards were reviewed separately. Variable-length guards now cover the complete changed spans. The leaf weapon helper retains its full 152-byte span; the round-clock helper retains its full 276-byte span.

The generated compatibility manifest contains 157 function fingerprints, 282 binding checks and 249 native addresses. Guards have not been disabled.

## Validation

Runtime, probe and native fixtures compile with warnings as errors. The probe reports 255/255 groups for the new snapshot; a deliberately changed guarded instruction is rejected (254/255, needs_review). Freecam, creator controls, Driving and acquired-build fixtures pass, including changed-code rejection. Native round-clock and late-join/round tracking tests pass. Capture and menu fixtures pass; the menu's simulated memory addresses and buffer bounds were updated, and palette output agrees with the new native palette within 0.000002 per channel.

Installer, updater, final package digests and compatibility-worker synchronization are recorded in the release validation record. Offline fixtures do not establish live Windows gameplay, FPS, remote sharing or Proton behavior. The owner's Steam installation is not changed by this release workflow.
