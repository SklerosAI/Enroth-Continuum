---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, save, menu]
---

# MM6 F4 Menu Slot Preflight

The [implementation packet](../project/MM6_F4_MENU_SLOT_PREFLIGHT.md) defines
a read-only v2 list that applies the same semantic primary/backup selection as
load preflight. It uses the selected candidate's HEAD and recovery flag, and
omits a slot when no candidate passes the caller's validator. Structural-only
`listRawSaveSlots()` is not a playable-slot inventory.

The visible MM6 load menu remains empty. A complete session and first-frame
admission validator is still needed before the menu can use the new list or
queue a filename. Synthetic tests verify selection and disk immutability;
they do not prove a real campaign load.

See also: [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [V2 Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md).
