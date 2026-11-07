---
type: synthesis
up: "[[MM6-F4-Spawned-Actor-Owner-Decision]]"
related: ["[[MM6-F4-Spawned-Actor-Section]]", "[[MM6-F4-EC6-V3-Manifest]]"]
tags: [mm6, f4, save, actor, audit]
---

# MM6 F4 Baseline Actor Save Audit

The [source-backed packet](../project/MM6_F4_BASELINE_ACTOR_SAVE_AUDIT.md)
shows why a spawned-only actor section cannot make a populated-map save
durable. The current DDM writer rejects actor count changes and copies only
selected changed runtime fields from original actors into retained native
records. Other actor changes can be omitted.

A disposable fresh-process headless full-list capture using the current
runtime section refused with `missing-mm6-resistance` (0/1); the spawned-only
first-map capture passed 1/1. The baseline native projection does not fill
that optional runtime resistance field for every actor. This is a guarded
finding, not a reason to invent default values.

The [detached v3 manifest](MM6-F4-EC6-V3-Manifest.md) remains a container
candidate only. A full active-actor owner with exact native provenance, or
a strict refusal of all unrepresented baseline mutations, is required
before a production save or fresh-child replay.

See also: [Spawned Actor Section](MM6-F4-Spawned-Actor-Section.md), [EC6 V3 Manifest](MM6-F4-EC6-V3-Manifest.md), [Bootstrap Respawn](MM6-F4-Raw-V2-Bootstrap-Respawn.md).
