---
type: synthesis
up: "[[MM6-F4-Chest-Generation]]"
tags: [mm6, f4, chest, artifact]
---

# MM6 F4 Chest Direct Artifact

The [public packet](../project/MM6_F4_CHEST_DIRECT_ARTIFACT.md) records the distinct original tier-seven chest source. Its bounded PE trace scans four overlapping 581-record windows, tests item IDs 400–429, but marks a local candidate mask by **scan slot index**. Artifact hits outside slots 400–430 address beyond that mask, so the previous simple party-ownership interpretation was unsupported. The subsequent candidate list contains IDs 400–430 inclusive; ID 430 has a non-artifact retained-table role. No tier-six generated-artifact flags or 13 cap are consulted here.

`mm6::pickDirectChestArtifactDetached` models only a safe subset over a caller-supplied synthetic memory image. It rejects unsafe mark indexes and MM7, then stages the original 31-candidate list, one RNG draw on success, and cleared/no-draw result when empty. Focused seeded cases cover slot-index masking, ID 430 and no cap. It cannot read a live party, choose an ownership policy or bypass the live MM6 chest guard.

The personal fork now explicitly chooses `mm6::pickDirectChestArtifactSafePolicy` for the detached [record transaction](../project/MM6_F4_CHEST_RECORD_TRANSACTION.md). It accepts caller-validated owned/generated flags for IDs 400–429, excludes their union, caps it at thirteen and stages a generated flag on selection. It omits ID 430 and never performs the original out-of-bounds slot-index write. This is an intentional compatibility divergence, not a claim of original direct-rule equivalence. Live ownership and generated-flag persistence remain gated.

Next: bound real party ownership/equipment and prove changed-state artifact persistence for the chosen policy. The original scan findings remain useful historical evidence but are not a runtime implementation target. The exact PE packet is ignored under `local-game-data/private-wiki/mm6/F4_CHEST_DIRECT_ARTIFACT_2026-11-06.md`.

See also: [MM6 F4 Chest Generation](MM6-F4-Chest-Generation.md), [MM6 F4 Chest Outer Plan](MM6-F4-Chest-Outer-Plan.md), [MM6 F4 Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md).
