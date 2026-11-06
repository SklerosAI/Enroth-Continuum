---
type: source-summary
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Save-Key-Matrix]]", "[[MM6-F4-Save-Remainder]]"]
tags: [mm6, f4, save, shop, guild, pe]
---

# MM6 F4 Shop Refresh PE Trace

The [source packet](../project/MM6_F4_SHOP_REFRESH_PE_TRACE.md) records a targeted read-only trace of the F3-identity-matched staged original MM6 executable. Exact addresses and row identities are kept in ignored owner-local evidence.

The original house parser stores retained column 15 in each of its 557 fixed-size records. Shop and spell-guild UI paths index that record by the active house key, regenerate stock when a per-house 64-bit deadline expires, and use column 15 as a day interval for the next deadline. Shop generation has a 12-position physical stride and type-specific visible limits of 6, 8, 12 and 6; guild generation uses a 12-position loop. This proves the observed refresh path and slot bounds, while item-record meaning and ordinary/special roles remain open.

The original `party.bin` writer and reader copy the same fixed global Party block. The traced clock and stock addresses lie inside it, establishing raw persistence in the original game. The [stock-read follow-up](MM6-F4-Shop-Stock-Tuple-and-Autosave.md) proves two menu banks and zero-ID display suppression. Adjacent stock/clock formulas coincide at a family boundary, so the effective end slots must be traced before defining MM6 runtime storage or `.ec6` fields. A native nonzero save sample and purchase write are still needed. Live MM6 SaveLoad remains guarded.

See also: [Shop Save Key Matrix](MM6-F4-Shop-Save-Key-Matrix.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Save Remainder](MM6-F4-Save-Remainder.md), [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md).
