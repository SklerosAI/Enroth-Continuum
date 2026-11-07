---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Spawned-Actor-Section]]", "[[MM6-F4-Pre-Event-GNPC-Replay]]"]
tags: [mm6, f4, save, ec6, manifest]
---

# MM6 F4 EC6 V3 Manifest

The [source-backed packet](../project/MM6_F4_EC6_V3_MANIFEST.md)
describes a detached exact EC6 v3 container: the nineteen strict v2 owners
plus required SPAW, GNPC and RNGS. Its linked v3/v2/SPAW focused suites
passed 12/12. V1/v2 readers retain their old meanings and v3 rejects them.

The later [baseline actor audit](MM6-F4-Baseline-Actor-Save-Audit.md) shows
that SPAW-only v3 cannot preserve changed original actors. Treat this as a
structural candidate. The [active-actor decision](MM6-F4-Active-Actor-Owner-Decision.md)
calls for replacing SPAW with full-list ACTR before any live promotion.

The SPAW owner is validated for canonical wire shape, including zero rows;
GNPC and RNGS use their bounded version-one codecs, and GNPC must name the
HEAD map. The manifest does not bind SPAW's first slot to the native active
DDM actor count, publish owners before map events, or write an atomic slot.
Those map-stage and fresh-child joins remain required before v3 admission.

See also: [Spawned Actor Section](MM6-F4-Spawned-Actor-Section.md), [Pre Event GNPC Replay](MM6-F4-Pre-Event-GNPC-Replay.md), [Atomic Save File](MM6-F4-Atomic-Save-File.md).
