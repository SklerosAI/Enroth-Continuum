---
type: decision
up: "[[Runtime-Isolation]]"
related: ["[[MM7-Self-Contained-Source]]", "[[Source-and-Privacy-Boundary]]"]
tags: [mm7, mac, saves, paths]
---

# MM7 Runtime Path Cutover

The [source packet](../project/MM7_RUNTIME_PATH_CUTOVER.md) defines the safe move from inherited Mac paths to Enroth Continuum's own MM7 data and profile roots.

Atlas and the direct Mac helper now default to `Game Data/MM7` and a fresh `Profiles/MM7-Atlas` under local Enroth Continuum Application Support. Atlas validates the engine's eight required archives, keeps a previous data preference when a new selection is invalid, and uses the fresh profile for launch, Vault, backups and folder access. Mac startup no longer copies saves from the data root. The source bundle label and Atlas license metadata identify Enroth Continuum MM7 and LGPL-3.0-or-later while retaining OpenEnroth credit.

Older save roots have distinct files with colliding names. Preserve each separately; stage a fresh profile only after private hash/backup verification and an explicit collision decision. Stage and test a candidate app before replacing the installed helper. The existing app and save roots remain untouched. MM7 game data was copied into the new private default folder and byte-verified against the original, which remains intact. Swift type checking, cached-library arm64 MM7/GameTest linking and a headless MM7 control through the new default data path passed. Visible launch and save/reload through a candidate app remain open.

Inherited internal config/log names, some window/CLI labels, clean offline build inputs and character transfer remain open. No game data, saves, detailed private inventory or installer bytes belong in this tracked wiki.

See also: [MM7 Self-Contained Source](MM7-Self-Contained-Source.md), [Runtime Isolation](Runtime-Isolation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md), [Agent Handoff](Agent-Handoff.md).
