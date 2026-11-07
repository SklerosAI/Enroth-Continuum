---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Child-Publication-Gate]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, npc, resources, reload]
---

# MM6 F4 NPC Resource Identity

The [implementation packet](../project/MM6_F4_NPC_RESOURCE_IDENTITY.md) defines a read-only check joining six loaded MM6 NPC resources to the fresh baseline, detached staged NPC state, live static tables and global dialogue topics.

`inspectNpcResourceIdentity()` reparses the title-selected resources, checks the static NPC rows, names, professions, map weights and topic text, and compares staged mutable NPC fields with v2 RESC. It does not mutate the ResourceManager, globals or RNG. Its result retains blockers for generated NPC identity after map load and unrepresented game RNG state; prior generated NPCs or name-cache state add explicit blockers. The caller must supply the MM6 ResourceManager because the resource object itself has no title getter. Two synthetic linked tests cover matching and divergent sources; a private loaded-table runtime invocation is still needed.

This check does not publish NPCs or prove their final map-generated identities. The [child publication gate](MM6-F4-Raw-Child-Publication-Gate.md) still refuses game-loop entry until these and other post-world owners are resolved.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Raw Child Publication Gate](MM6-F4-Raw-Child-Publication-Gate.md), [Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md).
