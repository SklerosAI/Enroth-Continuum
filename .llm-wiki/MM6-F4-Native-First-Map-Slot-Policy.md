---
type: synthesis
up: "[[MM6-F4-EC6-V5-Sprite-Runtime]]"
related: ["[[MM6-F4-Menu-Slot-Preflight]]", "[[MM6-F4-Atomic-Save-File]]"]
tags: [mm6, f4, save, profile, slot]
---

# MM6 F4 Native First-Map Slot Policy

The [project packet](../project/MM6_F4_NATIVE_FIRST_MAP_SLOT_POLICY.md)
defines a detached MM6 v5 manual-slot policy. It requires an exact
startup-resolved native MM6 root and requested root, a canonical non-symlink
profile/saves path, MM6 title, `saveNNN.ec6`, complete first-map v5 candidate
validation, and independently valid existing primary/backup files. A damaged,
foreign, other-map, backup-only or non-regular slot refuses before write.

The separate test-only numbered-slot atomic seam classifies an exact normal
return as `Committed`, an unchanged primary after a pre-replacement fault as
preflight-required, and a post-rename fault as indeterminate even when the
new primary is readable. Focused disposable units passed **2/2**, including
exact primary/backup states and collision refusals; MM7 Prs passed **1/1**.
The earlier real paused Save-menu capture and disposable writer/replay remain
separate evidence. This policy has no startup root handoff or normal
`doSavegame`/UI route, and it has not written a real paused candidate to a
numbered native slot. The existing owner-only candidate-slot guard remains.

See also: [V5 Sprite Runtime](MM6-F4-EC6-V5-Sprite-Runtime.md),
[Menu Slot Preflight](MM6-F4-Menu-Slot-Preflight.md),
[Atomic Save File](MM6-F4-Atomic-Save-File.md).
