# MM6 character event-bit branch

Status: bounded F4 Compare/Set seam, 2026-11-06. Later-map campaign consequences and live save/reload remain unobserved.

The F3-matched retained script ledger has 23 uses of raw MM6 event variable ID 213 across seven campaign resources: eleven Compare and twelve Set, with observed operands 1–41. The [MM6 event editor](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) calls ID 213 `CHARACTER_BIT`; [MMExtension](https://grayface.github.io/mm/ext/ref/) calls it `PlayerBits`. This is separate from the MM7 enum's same-named field at a different numeric ID.

The owner-local original executable still matches the accepted F3 SHA-256. Its raw-213 Compare jump target `0x44016e` tests bit `0x80 >> (operand % 8)` in character byte `operand / 8`; the Set target `0x440cdb` ORs the same mask into that byte. Both use a **zero-based wire bit index**. The runtime's `IndexedBitset<1, 512>` is one-based, and its reversed-bit snapshot maps runtime key 1 to wire bit 0. The MM6 adapter therefore validates operands 0–511 and accesses runtime key `operand + 1`. This corrects a potential one-bit shift; it does not alter the inherited MM7 route.

`mm6::CharacterEventBitState` implements checked, side-effect-free Compare and idempotent Set. The MM6 interpreter dispatches these before inherited MM7 variable validation, using its existing selected-character iterator. A Set prevalidates the operand before mutating any selected character. Add and Subtract for raw 213 remain guarded, as do other unresolved high IDs. Existing detached `.ec6` durable staging captures all four 64-byte character-bit fields, but live SaveLoad stays guarded.

Focused unit tests cover zero and last wire bit, idempotence, invalid no-mutation and first/last serialized byte masks. A separate synthetic headless GameTest checks Compare false, Set for the selected party and Compare true with scoped restoration. Arm64 syntax, style and diff checks passed; linked tests and actual campaign interaction remain pending integration. No proprietary event or executable bytes or private disassembly are committed.

See also: [variable dispatch](MM6_VARIABLE_DISPATCH_F2.md), [durable save staging](MM6_EC6_DURABLE_FIELDS_F4.md), [first-map event comparison](MM6_F4_FIRST_MAP_MONTH_EVENT.md).
