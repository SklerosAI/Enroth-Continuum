---
type: synthesis
up: "[[MM6-F4-Outdoor-Border-Trace]]"
related: ["[[MM6-F4-V5-Later-Outdoor]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, outdoor, travel, save]
---

# MM6 F4 Border Autosave Transaction

The [source audit](../project/MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md) defines what must be durable before MM6 can cross an outdoor border.

The current no-write location-change test proves that a loaded first-map session can switch to a second outdoor world and accept bounded input. A one-shot test intercept bypasses the old-map autosave; the old-map cache delta remains unchanged. It does not prove persistence or same-session return. `startMapTransition` and the foot-travel confirmation also call autosave through different paths, while normal MM6 `saveGame` is guarded. The MM7 `autoSave` contract cannot report durable success and its save preparation mutates map/cache state. The existing atomic v5 writer is limited to disposable candidate slots.

The next transaction must capture complete old-map and visited-map owners in a detached snapshot, normalize a copy of the party's out-of-bounds border position for safe reload, validate destination and MM6 eligibility, and make the disk write's durable result explicit. Only after accepted persistence may it publish the old-map cache, apply travel effects once and schedule the target. Failed capture or write must leave live Party, time, food, UI and map state unchanged. A post-rename sync error needs an explicit recovery policy because the new primary may already be present.

The current v5 schema has one active-map runtime set. Saving after entering a later map can discard prior-map actor/sprite/decoration runtime unless a visited-map owner or exact native projection retains it. A fresh-child cycle must reload both the target and old-map checkpoint and verify all owner families, plus an in-session return. Invalid/cancelled/flying/corner cases must perform no write. This remains a design audit: no production MM6 travel save or gameplay admission exists.

The original confirmation trace also establishes conditional rest: any food triggers full rest/heal, insufficient food adds a condition and no-rest day before food is floored at zero, and zero food skips healing. It wraps each crossed coordinate and recomputes target ground height. The inherited foot handler calls a time-advance helper that also heals even with zero food and lacks MM6 coordinate wrapping. The condition ID and independent original cancel branch still need proof. The source packet records the full transaction order before any live implementation.

See also: [Outdoor Border Trace](MM6-F4-Outdoor-Border-Trace.md), [V5 Later Outdoor](MM6-F4-V5-Later-Outdoor.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
