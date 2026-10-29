# Source distribution

Backend implementations, draft research/training internals, internal research tests
and investigation documents are excluded from this public snapshot. Their original
source is retained privately. Public client verification keys remain public; those
keys do not grant server signing authority. Previous public code remains in history.

This is a mixed-source project. Full runtime builds require the exact owner-provided
private component pinned by source-export.json. Verify it using
`python scripts/verify_public_source.py --private-source PATH`, then pass
`-PrivateSourceDirectory PATH` to Build-Runtime.ps1. Missing/extra files or changed
digests are rejected. Do not regenerate fingerprints or pins just to obtain a pass.
Full native binding/call audits also require `--private-source PATH`.

The public compatibility probe and Freecam/Driving fixtures build without private
inputs. Installer, updater, Linux source and payload approvals retain the current
stable v0.1.29 behavior. This cleanup does not publish a release or replace any assets.

The exact reviewed public inventory is hash-sealed in source-export.json. Release
packaging verifies it first. Do not put private inputs, owner settings, logs, real
recordings or credentials into the Git tree or source archive. Source export uses
an explicit owner-side allowlist; future files require review. Pattern scans augment
inventory/diff review and are not a proof against every possible kind of secret.
