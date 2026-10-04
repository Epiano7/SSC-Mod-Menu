# Compatibility hotfix worker

The mod already checks GitHub releases on Windows and Proton. Use that release
channel; no new mod endpoint or authentication-service change is needed.
This worker detects incompatibility without running the game. Detection is
implemented; unattended repair and publishing are deliberately not implemented.

## Read-only check on the Windows Steam PC

Use Python 3.11+ and an x64 MinGW C++17 compiler. From this repository:

```powershell
./scripts/Build-CompatibilityProbe.ps1 -Compiler 'C:/path/to/g++.exe' -Output './build/compatibility-probe.exe'
python ./scripts/check_game_update_test.py
python ./scripts/check_game_update.py --game 'C:/path/to/steamapps/common/SkillshotCity/SkillshotCity.exe' --steam-manifest 'C:/path/to/steamapps/appmanifest_308600.acf' --probe './build/compatibility-probe.exe' --output './build/compatibility-report.json'
```

The probe uses the same fingerprints and resolver as the runtime. It maps file
bytes into ordinary data memory, without loading DLLs, executing game code,
installing hooks, opening Steam sessions, or accessing user settings.

The wrapper requires the correct AppID, fully installed Steam state, completed
download/staging counts, and a matching installation path. It scans a temporary
snapshot and rejects results if the source executable or Steam manifest changes.
The report is replaced atomically. The supervisor should also require a stable
build ID and executable hash across two polls before invoking it; a polling
check cannot prevent Steam starting an update immediately afterward.

Exit codes: **0** compatible, **2** needs review, **3** input/probe/error.
JSON schema 1 includes app_id, build_id, game_sha256, probe_sha256, status,
required_groups, available_groups, details, and publish_allowed (always false).
Error reports have status/error instead of game fields. Never treat a missing,
stale, malformed report, timeout, or failed process as success.

`compatible` means the current runtime's native dependency checks passed. It
does not prove UI behavior, gameplay correctness, untracked ABI layout, or
installer acceptance of a new executable hash. Rebuild the probe whenever
runtime/compatibility*.h changes. Key stored results by game hash **and probe
hash**, so updating the mod rechecks the same game build.

## Server-agent handoff

Implement a separate compatibility-worker task on the Steam PC, independent of
the authorization service. Leave auth, beta, DNS and allowlists unchanged.

1. Use the existing signed-in Steam installation to receive game updates. Do
   not run a second Steam login or store its credentials. Do not kill a running
   game or force updates during a session. Configure Steam's update preferences
   with the owner; do not assume a guaranteed command-line queue API.
2. Watch appmanifest_308600.acf and SkillshotCity.exe. Wait for two stable polls,
   then run the checker above in a restricted worker account with a timeout.
   Persist only build/hash/report metadata. Deduplicate unchanged results;
   retry errors with backoff and alert on a new actionable failure.
3. For needs_review, preserve a private executable snapshot and the report;
   notify the owner/development agent with build ID, hashes and failed
   dependency groups. Never upload proprietary game files to a public repo.
4. Use an isolated source checkout for candidate fixes. Do not automatically
   regenerate compatibility_data.h and bless changed functions: generation
   records fingerprints, it does not validate ABI or semantics. Relocations
   already supported by the resolver need no address patch. Changed bodies,
   layouts, callsites or ambiguous matches require review and native tests.
5. Build reviewed changes with warnings as errors. Run compatibility tests
   against the new snapshot, the behavioral tests for affected modules,
   installer/rollback tests, and updater tests. The public repository excludes
   the owner's private test folder: a build saying tests are absent is NOT a
   passing gate. Arrange an approved test runner on the development PC or a
   separately approved CI suite; do not copy private fixtures into GitHub.
6. Produce both Windows and Proton release packages. Refresh the approved
   game/payload hashes only after validation. Follow docs/build.md and
   scripts/Build-Release.ps1. Use a new version consistently in Windows,
   runtime and Linux metadata; retain the standard asset names below.
7. Initially stop at a candidate report and ask the owner to approve publishing.
   Do not create PRs unless requested. After approval, publish a stable GitHub
   release with both uploaded assets and SHA256 digests. Do not replace an old
   release's assets or reuse its version. Keep previous releases for rollback.

Required asset names: `SSC-Mod-Menu-Setup.exe` and
`SSC-Mod-Menu-Linux.tar.gz`, in Epiano7/SSC-Mod-Menu, tag `vX.Y.Z`.
Existing clients reject drafts/prereleases, equal or older versions, unexpected
asset URLs, missing digests and corrupt downloads. The installed updater helper
also carries a version: distribute a full installer, not only runtime.dll.
Proton packages additionally enforce the exact supported game hash.

Publishing is not forced installation: users receive the existing update
notification and install through the existing flow. Live Windows and Proton
checks remain separate. A file-only check does not establish Proton gameplay
compatibility. Native Linux game binaries remain outside this PE/Proton probe.

## Initial validation (October 4, 2026)

Steam build 25711276, executable SHA256
`34bd0f95be0f2b649cf7d6fc0e5f5a0951400c9b9227ceccf84ea4bfe6aa573a`:
all eight groups passed (255). An older executable produced needs_review (exit
2), demonstrating a changed game is not automatically approved. Synthetic
monitor tests exercise failures without launching the game. These results do
not constitute live gameplay or publication approval.
