---
type: synthesis
up: "[[MM6-F4-Active-Actor-Owner-Decision]]"
related: ["[[MM6-F4-Baseline-Actor-Save-Audit]]", "[[MM6-F4-EC6-V3-Manifest]]"]
tags: [mm6, f4, save, actor]
---

# MM6 F4 Active Actor Section

The [source-backed packet](../project/MM6_F4_ACTIVE_ACTOR_SECTION.md)
records detached `ACTR` v1 for the full active MM6 actor list from PID slot
zero. It has a distinct tag and reuses the bounded canonical runtime record
codec, including exact MM6 resistances and a valid zero descriptor sentinel.

Linked ACTR/SPAW unit suites passed 8/8. A private-backed fresh-process
headless first-map ACTR encode/decode/re-encode probe passed 1/1 with ramfs
user state and no save write. The exact v3 manifest now requires ACTR, but
items/buffs, live
pre-event replay, and fresh-child save–quit–reload remain open.

See also: [Active Actor Owner Decision](MM6-F4-Active-Actor-Owner-Decision.md), [Baseline Actor Save Audit](MM6-F4-Baseline-Actor-Save-Audit.md), [Pre Event GNPC Replay](MM6-F4-Pre-Event-GNPC-Replay.md).
