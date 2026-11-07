---
type: synthesis
up: "[[MM6-F4-NPC-State]]"
related: ["[[MM6-F4-Post-Map-Generated-NPC-Links]]", "[[MM6-F4-Raw-Save-Runtime]]"]
tags: [mm6, f4, npc, save, rng]
---

# MM6 F4 Generated NPC Identity Save

The [source-backed packet](../project/MM6_F4_GENERATED_NPC_IDENTITY_SAVE.md)
describes a detached `GNPC` candidate for preserving generated MM6 NPC rows
and their active-map actor anchors.

The bounded codec retains map name, at most 100 complete logical NPC rows,
and strictly ordered actor slot/monster anchors. It rejects malformed fields
before return and offers a read-only post-map actor-link comparison. The
container recognizes its tag, but the exact EC6 v2 nineteen-section decoder
still rejects `GNPC`; no existing slot gains an unproved new meaning. Linked
synthetic validation is pending integration.

`GNPC` does not save the gameplay RNG. The generator uses `grng` for names,
portraits, reputation and professions before `onMapLoad()`. Current RNG APIs
cannot snapshot state after arbitrary draws. A future save version needs a
proved RNG continuation or an explicit tested fresh-entropy policy, plus a
row-restore point before replay-sensitive events. No live MM6 SaveLoad or
first-input admission follows from this detached codec.

See also: [Post-Map Generated NPC Links](MM6-F4-Post-Map-Generated-NPC-Links.md), [NPC State](MM6-F4-NPC-State.md), [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md).
