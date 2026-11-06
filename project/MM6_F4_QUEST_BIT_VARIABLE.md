# MM6 quest-bit event variable audit

Status: F4 bounded party-bit operation seam, 2026-11-06. Journal, audiovisual cues, live persistence and MM6 gameplay remain unverified.

The F3-matched retained EVT ledger has **611 uses of raw MM6 variable ID 16** on **208 distinct bit operands**, all within 1–376. The retained `quests.txt` has 512 numbered rows; 52 have visible text. Every active operand has a row, but 156 of the 208 used bits have blank or `0` text. Those are active hidden state candidates, not invalid operands. A reproducible [audit script](../scripts/mm6_f4_audit_quest_bits.py) verifies both inputs against the frozen F2 hashes and writes all 611 file/event/offset/operation/operand locators to an owner-only ignored report. It prints only aggregates.

| Operation | Active uses | Uses with visible text | Current inherited behavior |
| --- | ---: | ---: | --- |
| Compare | 276 | 15 | Tests `Party::_questBits[operand]`, but only inside character-target iteration. |
| Add | 59 | 43 | Sets the party bit; on a newly set text row, triggers inherited MM7 quest-book flash, animation and reaction. |
| Subtract | 92 | 54 | Clears the party bit and plays an inherited award reaction for each selected character, even if the bit was already clear. |
| Set | 184 | 8 | Sets the party bit; the notification path differs from Add and is inherited from MM7. |

The MM6-only `MM6QuestBitState` now bounds raw ID 16 to 1–512 and handles Compare, Add, Subtract and Set directly on the party bitset. The interpreter branches before inherited character iteration, so each instruction reads or changes the party bit once regardless of a character selector. Add and Set are idempotent set operations; Subtract clears. Invalid operands reject before mutation. The title-specific branch also covers scripted dialogue-topic comparison. MM7 retains its existing executor. The new path deliberately emits no inherited MM7 quest or award reaction; MM6 journal, animation and sound policy are still unproved. The active script records establish operand and operation coverage, not those presentation side effects.

The `.ec6` `QEST` field has a 64-byte quest-bit array covering IDs 1–512 and copy-stages it on a party copy. This is a bounded storage route, not live SaveLoad or proof of reset, journal and branch behavior. `QuestTable.cpp` loads the text separately; a blank text row does not erase or invalidate its bit.

The arm64 MM6, OpenEnroth and unit targets built, and two focused native tests passed for bounded, idempotent party-bit operations and pre-mutation rejection of 0, 513 and a wrong raw ID. The frozen audit covers all 611 active locators across the four operations; it does not execute those events. The `.ec6` `QEST` section already encodes and copy-stages all 512 bits, but live SaveLoad is unwired. Before calling this lane complete, exercise representative retained active operands from each operation and both text categories through the interpreter, prove selector independence and one mutation per instruction, and round-trip nonzero hidden and visible bits through the full `.ec6` transaction. F5 must observe a quest grant, a hidden-flag branch, a clear/regrant, journal cues and save/quit/relaunch/load while checking MM7 separately. Exact event sites stay in the ignored private report.
