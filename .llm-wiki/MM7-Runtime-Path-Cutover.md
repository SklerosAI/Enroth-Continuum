---
type: decision
up: "[[Runtime-Isolation]]"
related: ["[[MM7-Self-Contained-Source]]", "[[Source-and-Privacy-Boundary]]"]
tags: [mm7, mac, saves, paths]
---

# MM7 Runtime Path Cutover

The [source packet](../project/MM7_RUNTIME_PATH_CUTOVER.md) defines the safe move from inherited Mac paths to Enroth Continuum's own MM7 data and profile roots.

Atlas and the direct Mac helper now default to `Game Data/MM7` and a fresh `Profiles/MM7-Atlas` under local Enroth Continuum Application Support. Atlas validates the engine's eight required archives, keeps a previous data preference when a new selection is invalid, and uses the fresh profile for launch, Vault, backups and folder access. Mac startup no longer copies saves from the data root. The source bundle label and Atlas license metadata identify Enroth Continuum MM7 and LGPL-3.0-or-later while retaining OpenEnroth credit.

Older save roots had distinct files with colliding names. Preserve each separately and make an explicit collision decision before any migration. MM7 game data was copied into the new private default folder and byte-verified against the original, which remains intact. Swift type checking, cached-library arm64 MM7/GameTest linking and a headless MM7 control through the new default data path passed. The installed Atlas now uses `Profiles/MM7-Atlas`; earlier saves were staged there under the owner's local migration decision.

The Mac MM7 helper now writes `enroth-continuum-mm7.ini` and a rotating `enroth-continuum-mm7` log in its separate profile, and defaults the window title to Enroth Continuum MM7. If the selected profile has only a legacy config, it reads that file but writes the new name, leaving the original intact. The preferred `ENROTH_CONTINUUM_MM7_PATH` override wins over the legacy alias; the private MM7 data root precedes cwd. A local-only arm64 MM7/GameTest build and focused MM7/MM6 headless controls passed after this change. The [clean tracked-source build](MM7-Clean-Offline-Build.md) covers a separate source-owned build route. Some inherited internal/CLI names and character transfer remain open. No game data, saves, detailed private inventory or installer bytes belong in this tracked wiki.

On 2026-11-07 the installed Release helper was replaced after rebuilding against the Apple OpenGL framework. The staged and installed bundles passed code-signature validation, `otool -L` found no XQuartz dependency, and a visible Atlas launch reached the MM7 title menu and returned normally. The existing private data and save profile were not rewritten by this install; the previous app bundle is retained in an ignored local backup. The test build passed focused `Prs.Pr2956`, `Issues.Issue840` and `Prs.Pr2354` controls 3/3. This checkpoint does not establish a new save/reload result or a source-built media-library cutover for the installed helper.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
