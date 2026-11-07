---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Post-Rebuild-PID-Slots]]", "[[MM6-F4-Persistent-Event-Publication]]", "[[MM6-F4-V2-Fresh-Child-Owner-Audit]]"]
tags: [mm6, f4, save, reload, world]
---

# MM6 F4 Post-World Owner Validator

The [implementation packet](../project/MM6_F4_POST_WORLD_OWNER_VALIDATOR.md) describes a read-only MM6 v2 check for the one-shot post-`DoPrepareWorld(true)` test hook. It joins loaded map ID/name/type, saved active plan and CHST name, final actor/sprite slot anchors, final persistent sidecar/generic arrays, and the complete post-generation chest sidecar, LCG, artifact ledger and twenty live chest records. The chest check reruns the existing detached generator from the saved CHST owner and compares exact final bytes; occupied item replay requires its 28-byte raw provenance. Three linked synthetic tests passed for indoor/outdoor agreement and late refusal. No live hook currently calls this validator.

Matching actor/sprite anchors do not prove saved-delta selection or absence of slot reuse. Event replay may legitimately change a proved byte and be refused. Renderer/UI/cursor, NPC static/generated state, BEAC/IFOV, action queues and recovery are separate gates; active VISU remains prohibited. Normal MM6 SaveLoad and Atlas Play remain guarded.

See [Post-Rebuild PID Slots](MM6-F4-Post-Rebuild-PID-Slots.md), [Persistent Event Publication](MM6-F4-Persistent-Event-Publication.md), and [V2 Fresh-Child Owner Audit](MM6-F4-V2-Fresh-Child-Owner-Audit.md).
