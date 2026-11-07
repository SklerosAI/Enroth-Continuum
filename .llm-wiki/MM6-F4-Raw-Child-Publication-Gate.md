---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-V2-QuickLoad-Callback-Gate]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, reload, admission]
---

# MM6 F4 Raw Child Publication Gate

The [implementation packet](../project/MM6_F4_RAW_CHILD_PUBLICATION_GATE.md) describes a test-only read-only admission check for one detached native v2 session before any live owner assignment.

`inspectRawChildPublication()` replays the physical inventory through the staged Party using the loaded MM6 item table and rechecks the raw held cursor, shop/clock owners, UIST, inactive VISU, no-beacon policy, MAPS cache, mutable NPC rows, timer/overlays, CHST and persistent map bytes. Divergence throws before publication. A coherent candidate receives typed blockers for unresolved mouse image and offset, static NPC resources, active world/chest/PID identity, post-world events, BEAC/IFOV reset, timers/triggers, actions/UI and the missing post-world hook. `canEnterGameLoop()` remains false. The corrected linked synthetic suite passed 2/2; its first integration run exposed a test fixture that had not installed the supplied table as the active `pItemTable`.

The fresh-child ordering audit found one precise recovery loss: `RCVR` can stage a nonzero Armageddon timer, but world preparation clears it before the first input frame. The read-only inspector now checks the staged Party timer against decoded `RCVR` and returns a typed `WorldPrepareClearsArmageddonTimer` blocker for a coherent nonzero value. A post-world restore/check of the timer and its coupled damage/force state is required before that save can be admitted. This narrows the issue without publishing any live owner.

This is an honest refusal gate, not a partial live loader. A future MM6-only one-shot hook after `DoPrepareWorld(true)` must validate the rebuilt world before first input, as detailed in the [fresh-child owner audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md). The [caught QuickLoad callback test](MM6-F4-V2-QuickLoad-Callback-Gate.md) exercises the current refusal without queuing a Main Menu load event.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md), [V2 QuickLoad Callback Gate](MM6-F4-V2-QuickLoad-Callback-Gate.md).
