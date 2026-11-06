---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Shop-Save-Key-Matrix]]", "[[MM6-F4-Shop-Stock-Tuple-and-Autosave]]"]
tags: [mm6, f4, save, shop, guild, clock]
---

# MM6 F4 Shop Guild Clock Save

The [SCLK packet](../project/MM6_F4_SHOP_GUILD_CLOCK_SAVE.md) records the eighteenth required `.ec6` section, limited to original-MM6 source-proven refresh clocks.

The hash-matched retained 557-row house table and original refresh trace support shop row IDs 1–47 and spell-guild IDs 119–140 with 64-bit deadline words. Shop 47 and guild 119 alias one physical word, so `SCLK` stores 68 unique words, not two separate boundary clocks. It requires a ready MM6 title-local owner, checks exact length/version and stages a copy without changing live Party or map state. Old provisional seventeen/sixteen-section candidates reject.

Stock banks and bounty tuples remain blocked by the missing purchased-slot write, complete item record meaning, edge-row behavior and native nonzero save sample. The inherited MM7-indexed arrays still reject when nonempty. The current route has no MM6 service writers or all-owner commit; live SaveLoad remains guarded.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Shop Save Key Matrix](MM6-F4-Shop-Save-Key-Matrix.md), [Shop Stock Tuple and Autosave](MM6-F4-Shop-Stock-Tuple-and-Autosave.md).
