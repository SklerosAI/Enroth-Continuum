# MM6 quest-bit event variable audit

Status: F4 operation and persistence handoff, 2026-11-06. No new quest-bit runtime mutation or MM6 game launch is claimed.

The F3-matched retained EVT ledger has **611 uses of raw MM6 variable ID 16** on **208 distinct bit operands**, all within 1–376. The retained `quests.txt` has 512 numbered rows; 52 have visible text. Every active operand has a row, but 156 of the 208 used bits have blank or `0` text. Those are active hidden state candidates, not invalid operands. A reproducible [audit script](../scripts/mm6_f4_audit_quest_bits.py) verifies both inputs against the frozen F2 hashes and writes all 611 file/event/offset/operation/operand locators to an owner-only ignored report. It prints only aggregates.

| Operation | Active uses | Uses with visible text | Current inherited behavior |
| --- | ---: | ---: | --- |
| Compare | 276 | 15 | Tests `Party::_questBits[operand]`, but only inside character-target iteration. |
| Add | 59 | 43 | Sets the party bit; on a newly set text row, triggers inherited MM7 quest-book flash, animation and reaction. |
| Subtract | 92 | 54 | Clears the party bit and plays an inherited award reaction for each selected character, even if the bit was already clear. |
| Set | 184 | 8 | Sets the party bit; the notification path differs from Add and is inherited from MM7. |

The current MM7 value validator allows quest bits 1–512, so all shipped operands are in range. Raw ID 16 remains a provisional shared candidate in [the variable guard](MM6_F4_VARIABLE_GUARD.md): numeric overlap does not prove MM6 operation, target-selection or notification semantics. In particular, the party bit is currently read or changed inside `iterateCharacters`; a party selector can repeat character-specific cues, while an empty selector can suppress a party-wide Compare or write. Add and Set both write a bit in the inherited code, but their distinct cue paths and the active hidden/text mix require a title-specific decision. Subtract's unconditional award reaction is also unproved for MM6. The active script records establish operands and operation counts, not intended side effects.

The `.ec6` `QEST` field has a 64-byte quest-bit array covering IDs 1–512 and copy-stages it on a party copy. This is a bounded storage route, not live SaveLoad or proof of reset, journal and branch behavior. `QuestTable.cpp` loads the text separately; a blank text row does not erase or invalidate its bit.

Before enabling a title-specific executor, verify party versus character selection, Compare branch polarity, Add/Set idempotence and cue policy, Subtract on set and clear bits, and whether the visible text rows really open/update the MM6 journal. Offline acceptance should replay representative retained active operands from each operation and both text categories, reject 0 and 513 before mutation, prove one party mutation per instruction, cover all 611 locators without MM7 fallthrough, and round-trip a nonzero hidden and visible bit through `QEST`. F5 must observe a quest grant, a hidden-flag branch, a clear/regrant, journal cues and save/quit/relaunch/load while checking MM7 separately. Exact event sites stay in the ignored private report.
