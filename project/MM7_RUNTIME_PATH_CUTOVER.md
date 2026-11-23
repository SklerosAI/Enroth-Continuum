# MM7 Mac runtime path cutover

Status: **installed Atlas uses the Enroth Continuum data and profile paths** (cut over 2026-11-07, reinstalled 2026-11-22). Under the owner's app replacement policy in `AGENTS.md`, a verified replacement deletes the old app, so no old app is kept.

## Source state

- Atlas defaults MM7 game data to `~/Library/Application Support/Enroth Continuum/Game Data/MM7` and its user path to the fresh `Profiles/MM7-Atlas` sibling. Its Vault, backup and folder actions derive from that same user path. A saved `data.mm7` preference may still explicitly select a different folder.
- The direct Mac MM7 helper resolves the same default data and user locations. It checks the private MM7 data root before the current directory on macOS. `ENROTH_CONTINUUM_MM7_PATH` is its preferred environment override; the older `OPENENROTH_MM7_PATH` remains accepted when the new override is unset. Atlas passes both paths explicitly when launching its bundled helper.
- Atlas checks all eight MM7 archives required by the engine and retains the previous preference if a newly selected folder fails validation. Mac MM7 no longer copies a data folder's `saves/` into an empty user profile at startup.
- The source bundle label is `Enroth Continuum MM7`; Atlas license metadata now says LGPL-3.0-or-later and retains OpenEnroth contributor credit. The Mac MM7 helper uses `enroth-continuum-mm7.ini`, `logs/enroth-continuum-mm7.log` and the default window title `Enroth Continuum MM7`. If an explicitly selected MM7 profile lacks the new config but has `openenroth.ini`, the helper reads that legacy file and later saves only the new name, leaving the legacy file intact. A custom window title remains intact. MM6 and non-Mac paths retain their existing names. Inherited internal symbols and some CLI/build strings remain for a later scoped audit.

## Save migration

The older local MM7 save locations hold distinct saves with colliding names. They are never merged by filename, and no startup auto-copy picks a winner. Saves reach `Profiles/MM7-Atlas` only through an explicit per-collision choice or a separate import mechanism, and the original save folders are left intact. Game data and saves stay out of Git.

## Verification

The source path change passed Swift type checking and a cached-library arm64 `MM7` and GameTest build. Extracted MM7 game data was copied privately to the new default folder and every regular file was byte-verified against the original. The original remains intact. A headless MM7 control then passed through the new default path.

The later MM7-only identity change linked `MM7` and GameTest in an isolated arm64 Release build using the repository-owned media-library prefix with downloads disabled. A headless MM7 control passed 1/1 and saved the new config name in its disposable user filesystem. An MM6 control passed 1/1 and retained `openenroth.ini`. In a separate disposable profile, the direct MM7 helper read a legacy config, created a rotating log with the new name and left the legacy file intact. A preferred environment override took precedence and failed closed when invalid. The legacy override still resolved a valid MM7 path when the new one was unset.

See [MM7 source autonomy](MM7_SELF_CONTAINED_SOURCE.md), [runtime isolation](../.llm-wiki/Runtime-Isolation.md), [credits](../CREDITS.md) and [upstream relationship](UPSTREAM.md).
