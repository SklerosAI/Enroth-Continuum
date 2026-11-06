---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, quests, saves]
---

# MM6 F4 Quest Bit Variable

The [quest-bit packet](../project/MM6_F4_QUEST_BIT_VARIABLE.md) audits raw MM6 EVT ID 16 without enabling a new executor. F3-matched retained scripts use it 611 times across Compare, Add, Subtract and Set, with 208 distinct operands inside the 512-row quest table. Fifty-two used bits have visible text; 156 are active hidden-state candidates. The [owner-local report generator](../scripts/mm6_f4_audit_quest_bits.py) checks frozen input hashes and writes every source locator to the ignored private companion.

The MM6 interpreter now routes Compare, Add, Subtract, Set and dialogue-topic comparison through a bounded party-bit helper, once per instruction and without inherited MM7 character cues. Add/Set are idempotent set operations, and Subtract clears. Two focused native tests passed; the arm64 MM6/MM7/unit targets built. The `.ec6` `QEST` section can copy-stage all 512 bits, but live SaveLoad and original branch/journal semantics are unverified. Keep raw ID 16 provisional until representative retained events, selectors, cues and persistence are checked. Blank quest text does not mean an active bit is invalid.

See also: [MM6 F4 Autonote Variables](MM6-F4-Autonote-Variables.md), [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 F4 Known Save Assembly](MM6-F4-Known-Save-Assembly.md).
