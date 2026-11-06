---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, autonotes]
---

# MM6 F4 Autonote Variables

The [autonote packet](../project/MM6_F4_AUTONOTE_VARIABLE.md) establishes a narrow MM6-only executor for raw EVT ID 205. The F3-matched private 127-ID ledger has 101 active uses (97 Set, four Add), all operands in the 1–128 rows of the singular `autonote.txt` resource that the runtime loads. The similarly named plural table is distinct. Exact script sites and table contents remain in ignored `local-game-data/`.

ID 205 is party-scoped. Checked Set/Add, Subtract and Compare operate once on `Party::_autonoteBits`, reject operands outside 1–128 before mutation, and leave the 208-bit field's other rows intact. The existing `.ec6` `QEST` copy-staging carries that field, but live SaveLoad remains blocked. IDs 206–226 remain guarded; the MM7 variable path is unchanged.

This seam does not yet show the new note animation, book flash or sound. F5 must compare the visible note book and notification against a real event and verify durability across save/relaunch/load. Follow the [variable-dispatch contract](Source-MM6-Variable-Dispatch-F2.md) for the other active ID families.

See also: [MM6 F4 Quest Bit Variable](MM6-F4-Quest-Bit-Variable.md).
