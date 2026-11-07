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

A disposable fresh-process headless full-list capture initially refused with
`missing-mm6-resistance`. The MM6 loader now restores the exact six-byte
tuple from each native actor record; its focused test passed 1/1. The same
full-list probe next identified a zero descriptor on an actor that can act.
The codec now preserves only that exact sentinel, with a valid embedded
monster identity and canonical replay; the full-list probe passed 1/1.
The spawned-only first-map capture also passed 1/1. No default resistance
or broad actor identity relaxation was used.

The [detached v3 manifest](MM6-F4-EC6-V3-Manifest.md) remains a container
candidate only. A full active-actor owner with exact native provenance, or
a strict refusal of all unrepresented baseline mutations, is required
before a production save or fresh-child replay. The
[active-actor decision](MM6-F4-Active-Actor-Owner-Decision.md) now proposes
full-list `ACTR` coverage; that candidate still needs live replay and
complete item/buff handling.

See also: [Spawned Actor Section](MM6-F4-Spawned-Actor-Section.md), [EC6 V3 Manifest](MM6-F4-EC6-V3-Manifest.md), [Bootstrap Respawn](MM6-F4-Raw-V2-Bootstrap-Respawn.md).
