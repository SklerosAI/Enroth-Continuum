---
type: synthesis
up: "[[MM6-F4-Item-Event-Writes]]"
tags: [mm6, f4, events, items, executable, transaction]
---

# MM6 F4 Item Event PE

The [public PE packet](../project/MM6_F4_ITEM_EVENT_PE_TRANSACTION.md) and tracked PE checker follow original MM6 opcode/variable dispatch from an owner-local executable matching the accepted F3 identity. Exact code bytes/disassembly are ignored locally. The original Add creates a zeroed identified item, relocates a prior cursor item and marks MM6 generated-artifact IDs 400–429. Original Subtract targets the selected character or each party member, checks backpack grid before cursor and leaves a miss unchanged. The inherited MM7 item write path does not match these semantics.

The MM6 runtime now executes only an empty-cursor, single-recipient, nonartifact Add and a preflighted selected-character Subtract. It guards occupied-cursor relocation, artifact grants and ambiguous duplicate backpack matches. Among 290 retained Adds, 288 have eligible static operand/selector; runtime cursor occupancy can still reject them. All 156 retained Subtract operands are statically eligible; duplicate backpack state remains guarded. Six focused native comparison/write tests passed. Live save/load and F5 visual play remain open.

Next agents should read the private companion `local-game-data/private-wiki/mm6/F4_ITEM_EVENT_PE_2026-11-06.md` and complete cursor fallback order, MM6 artifact flag persistence, duplicate-grid priority and graphical reward/handoff acceptance before claiming campaign progress.
