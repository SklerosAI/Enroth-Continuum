# MM7 Mac runtime path cutover

Status: **source defaults staged; installed app and private files unchanged** (2026-11-07). This packet covers a collision-safe move from inherited OpenEnroth local paths to Enroth Continuum's own MM7 data and profile paths. It does not make the clean offline build or character transfer complete.

## Source state

- Atlas defaults MM7 game data to `~/Library/Application Support/Enroth Continuum/Game Data/MM7` and its user path to the fresh `Profiles/MM7-Atlas` sibling. Its Vault, backup and folder actions derive from that same user path. A saved `data.mm7` preference may still explicitly select a different folder.
- The direct Mac MM7 helper resolves the same default data and user locations. Atlas passes both paths explicitly when launching its bundled helper.
- Atlas checks all eight MM7 archives required by the engine and retains the previous preference if a newly selected folder fails validation. Mac MM7 no longer copies a data folder's `saves/` into an empty user profile at startup.
- The source bundle label is `Enroth Continuum MM7`; Atlas license metadata now says LGPL-3.0-or-later and retains OpenEnroth contributor credit. Inherited internal symbols, config/log names and some window/CLI strings still need a scoped identity audit before the final cutover.

## Private migration gate

The older local MM7 save locations contain distinct saves with colliding names. Never merge them by filename, overwrite one with the other, or let a startup auto-copy choose a winner. Keep both original locations and the installed app as rollback sources.

1. Record private hashes and a backup of each source data/profile root separately. Verify a fresh target is empty and reject symlinks or paths outside the intended private roots.
2. Copy extracted game data into a new staging sibling of `Game Data/MM7`, verify relative files and hashes, then atomically rename it. Keep game data and saves out of Git.
3. Stage saves into a **fresh** `Profiles/MM7-Atlas` only after an explicit per-collision choice or a separate import mechanism. Verify the staged profile and keep both originals. Do not automatically reverse-merge on rollback.
4. Build and stage a candidate Atlas/helper separately from the installed app. With the new paths, check a headless MM7 control, visible launch and map rendering, audio/video, existing save load, new save, quit, relaunch and reload. Confirm the MM6 and MM8 sibling slots and private profiles are untouched.
5. Only after those checks, switch the installed app and its saved data preference. Preserve the old app and roots until a later real play check succeeds. Roll back by restoring the old app/preference; leave all save folders intact.

The source-only path change passed Swift type checking, a cached-library arm64 `MM7` and GameTest build, and an MM7 headless control using an explicit existing private data path. The new default data folder is not populated yet, so the default-path launch is not claimed as working. No installed app or private save was moved in this pass.

See [MM7 source autonomy](MM7_SELF_CONTAINED_SOURCE.md), [runtime isolation](../.llm-wiki/Runtime-Isolation.md), [credits](../CREDITS.md) and [upstream relationship](UPSTREAM.md).
