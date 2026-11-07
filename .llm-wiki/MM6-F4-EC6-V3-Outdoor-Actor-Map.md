---
type: synthesis
up: "[[MM6-F4-EC6-V3-Manifest]]"
related: ["[[MM6-F4-Active-Actor-Section]]", "[[MM6-F4-Post-World-Owner-Validator]]"]
tags: [mm6, f4, save, actor, map]
---

# MM6 F4 EC6 V3 Outdoor Actor Map

The [source-backed packet](../project/MM6_F4_EC6_V3_OUTDOOR_ACTOR_MAP.md)
records a detached read-only join between the exact v3 manifest, all 67
native map pairs, active CHST sidecar and full-list ACTR.

The linked v3 map/manifest/ACTR suites passed 10/10. The helper requires
matching active map identity, native actor anchors and sidecar shape, a
nonzero day that will not respawn on save load, and an ACTR list within the
runtime actor cap. It permits changed actor HP in ACTR while DDM retains
older bytes, because ACTR is the proposed gameplay owner.

No actors are published. GNPC links, pre-event RNG replay, post-world
resource/event agreement and first-input admission remain separate.

See also: [EC6 V3 Manifest](MM6-F4-EC6-V3-Manifest.md), [Active Actor Section](MM6-F4-Active-Actor-Section.md), [Post-World Owner Validator](MM6-F4-Post-World-Owner-Validator.md).
