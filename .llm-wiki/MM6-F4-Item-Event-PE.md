---
type: synthesis
up: "[[MM6-F4-Item-Event-Writes]]"
tags: [mm6, f4, events, items, executable, transaction]
---

# MM6 F4 Item Event PE

The [public PE packet](../project/MM6_F4_ITEM_EVENT_PE_TRANSACTION.md) and tracked PE checker follow original MM6 opcode/variable dispatch from an owner-local executable matching the accepted F3 identity. Exact code bytes/disassembly are ignored locally. The original Add creates a zeroed identified item, relocates a prior cursor item and marks MM6 generated-artifact IDs 400–429. Original Subtract targets the selected character or each party member, checks backpack grid before cursor and leaves a miss unchanged. The inherited MM7 item write path does not match these semantics.

The MM6 runtime executes single-recipient, nonartifact Add with an empty cursor or an occupied cursor whose prior item fits one inventory. The occupied path tries active character first, then indices 0–3, copies exact item fields before changing the cursor, and rejects a full party before mutation. It guards all-inventories-full ground drop, artifact grants and ambiguous duplicate backpack matches. Among 290 retained Adds, 288 have eligible static operand/selector; occupied-cursor capacity can still reject them. All 156 retained Subtract operands are statically eligible; duplicate backpack state remains guarded. Eight focused native comparison/write tests passed. Live save/load and F5 visual play remain open.

Next agents should read the private companion `local-game-data/private-wiki/mm6/F4_ITEM_EVENT_PE_2026-11-06.md` and recover the ground-object constructor at `0x42A730`, prove checked object-descriptor/map-sprite persistence, add MM6 artifact flag persistence, resolve duplicate-grid priority, and visually verify reward/handoff and cursor rollback before claiming campaign progress. The event path is title gated; MM7 keeps its existing executor.
