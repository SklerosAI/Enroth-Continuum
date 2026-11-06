---
up: "[[MM6-F4-Chest-Generation]]"
---

# MM6 F4 Chest Direct Artifact

The [public packet](../project/MM6_F4_CHEST_DIRECT_ARTIFACT.md) records the distinct original tier-seven chest source. Its bounded PE trace scans four overlapping 581-record windows, tests item IDs 400–429, but marks a local candidate mask by **scan slot index**. Artifact hits outside slots 400–430 address beyond that mask, so the previous simple party-ownership interpretation was unsupported. The subsequent candidate list contains IDs 400–430 inclusive; ID 430 has a non-artifact retained-table role. No tier-six generated-artifact flags or 13 cap are consulted here.

`mm6::pickDirectChestArtifactDetached` models only a safe subset over a caller-supplied synthetic memory image. It rejects unsafe mark indexes and MM7, then stages the original 31-candidate list, one RNG draw on success, and cleared/no-draw result when empty. Focused seeded cases cover slot-index masking, ID 430 and no cap. It cannot read a live party, choose an ownership policy or bypass the live MM6 chest guard.

An explicit optional `mm6::pickDirectChestArtifactSafePolicy` instead accepts caller-validated owned/generated flags for IDs 400–429, excludes their union, caps it at 13 and stages a generated flag on selection. It omits ID 430. This is a deterministic **compatibility choice**, not the original direct rule; its different cap, ownership and persistence implications need a gameplay decision before any live adapter.

Next: identify the scanned memory object and original out-of-mask consequences, bound real party ownership/equipment, decide ID 430 and duplication policy, and prove changed-state artifact persistence. The exact PE packet is ignored under `local-game-data/private-wiki/mm6/F4_CHEST_DIRECT_ARTIFACT_2026-11-06.md`.
