---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Active-Actor-Section]]", "[[MM6-F4-Pre-Event-GNPC-Replay]]"]
tags: [mm6, f4, save, ec6, manifest]
---

# MM6 F4 EC6 V3 Manifest

The [source-backed packet](../project/MM6_F4_EC6_V3_MANIFEST.md)
describes a detached exact EC6 v3 container: the nineteen strict v2 owners
plus required ACTR, GNPC and RNGS. Its linked v3/v2/ACTR focused suites
passed 11/11. V1/v2 readers retain their old meanings and v3 rejects them.

The later [baseline actor audit](MM6-F4-Baseline-Actor-Save-Audit.md) shows
that the earlier SPAW-only candidate could not preserve changed original
actors. The [active-actor decision](MM6-F4-Active-Actor-Owner-Decision.md)
is now reflected in this manifest, but it remains a structural candidate.

The ACTR owner is validated for canonical full-list wire shape, including an empty list;
GNPC and RNGS use their bounded version-one codecs, and GNPC must name the
HEAD map. The manifest does not bind ACTR's baseline portion to the native
DDM actors, publish owners before map events, or write an atomic slot.
Those map-stage and fresh-child joins remain required before v3 admission.

See also: [Active Actor Section](MM6-F4-Active-Actor-Section.md), [Pre Event GNPC Replay](MM6-F4-Pre-Event-GNPC-Replay.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md).
