# MM7 Mac runtime path cutover

Status: **source defaults staged; private game data copied and verified; installed app and saves unchanged** (2026-11-07). This packet covers a collision-safe move from inherited OpenEnroth local paths to Enroth Continuum's own MM7 data and profile paths. The opt-in clean tracked-source Mac build now passes; graphical play, save/reload, installed-app cutover and character transfer remain open.

## Source state

- Atlas defaults MM7 game data to `~/Library/Application Support/Enroth Continuum/Game Data/MM7` and its user path to the fresh `Profiles/MM7-Atlas` sibling. Its Vault, backup and folder actions derive from that same user path. A saved `data.mm7` preference may still explicitly select a different folder.
- The direct Mac MM7 helper resolves the same default data and user locations. It checks the private MM7 data root before the current directory on macOS. `ENROTH_CONTINUUM_MM7_PATH` is its preferred environment override; the older `OPENENROTH_MM7_PATH` remains accepted when the new override is unset. Atlas passes both paths explicitly when launching its bundled helper.
- Atlas checks all eight MM7 archives required by the engine and retains the previous preference if a newly selected folder fails validation. Mac MM7 no longer copies a data folder's `saves/` into an empty user profile at startup.
- The source bundle label is `Enroth Continuum MM7`; Atlas license metadata now says LGPL-3.0-or-later and retains OpenEnroth contributor credit. The Mac MM7 helper uses `enroth-continuum-mm7.ini`, `logs/enroth-continuum-mm7.log` and the default window title `Enroth Continuum MM7`. If an explicitly selected MM7 profile lacks the new config but has `openenroth.ini`, the helper reads that legacy file and later saves only the new name, leaving the legacy file intact. A custom window title remains intact. MM6 and non-Mac paths retain their existing names. Inherited internal symbols and some CLI/build strings remain for a later scoped audit.

## Private migration gate

The older local MM7 save locations contain distinct saves with colliding names. Never merge them by filename, overwrite one with the other, or let a startup auto-copy choose a winner. Keep both original locations and the installed app as rollback sources.

1. Record private hashes and a backup of each source data/profile root separately. Verify a fresh target is empty and reject symlinks or paths outside the intended private roots.
2. Copy extracted game data into a new staging sibling of `Game Data/MM7`, verify relative files and hashes, then atomically rename it. Keep game data and saves out of Git.
3. Stage saves into a **fresh** `Profiles/MM7-Atlas` only after an explicit per-collision choice or a separate import mechanism. Verify the staged profile and keep both originals. Do not automatically reverse-merge on rollback.
4. Build and stage a candidate Atlas/helper separately from the installed app. With the new paths, check a headless MM7 control, visible launch and map rendering, audio/video, existing save load, new save, quit, relaunch and reload. Confirm the MM6 and MM8 sibling slots and private profiles are untouched.
5. Only after those checks, switch the installed app and its saved data preference. Preserve the old app and roots until a later real play check succeeds. Roll back by restoring the old app/preference; leave all save folders intact.

The source path change passed Swift type checking and a cached-library arm64 `MM7` and GameTest build. Extracted MM7 game data was copied privately to the new default folder and every regular file was byte-verified against the original; the original remains intact. A headless MM7 control then passed through the new default path. The installed app, preferences and private saves were not moved, so visible launch and save/reload through a candidate app remain open.

The later MM7-only identity change linked `MM7` and GameTest in an isolated arm64 Release build using the repository-owned media-library prefix with downloads disabled. A headless MM7 control passed 1/1 and saved the new config name in its disposable user filesystem; an MM6 control passed 1/1 and retained `openenroth.ini`. In a separate disposable profile, the direct MM7 helper read a legacy config, created a rotating log with the new name and left the legacy file intact. A preferred environment override took precedence and failed closed when invalid; the legacy override still resolved a valid MM7 path when the new one was unset. These checks did not launch a visible game or alter the installed app and saves.

See [MM7 source autonomy](MM7_SELF_CONTAINED_SOURCE.md), [runtime isolation](../.llm-wiki/Runtime-Isolation.md), [credits](../CREDITS.md) and [upstream relationship](UPSTREAM.md).
