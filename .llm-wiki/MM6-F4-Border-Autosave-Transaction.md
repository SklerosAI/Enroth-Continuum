---
type: synthesis
up: "[[MM6-F4-Outdoor-Border-Trace]]"
related: ["[[MM6-F4-V5-Later-Outdoor]]", "[[MM6-F4-Save-Transaction-Checklist]]"]
tags: [mm6, f4, outdoor, travel, save]
---

# MM6 F4 Border Autosave Transaction

The [source audit](../project/MM6_F4_BORDER_AUTOSAVE_TRANSACTION.md) defines what must be durable before MM6 can cross an outdoor border.

The current no-write location-change test proves that a loaded first-map session can switch to a second outdoor world and accept bounded input. A one-shot test intercept bypasses the old-map autosave; the old-map cache delta remains unchanged. It does not prove persistence or same-session return. `startMapTransition` and the foot-travel confirmation also call autosave through different paths, while normal MM6 `saveGame` is guarded. The MM7 `autoSave` contract cannot report durable success and its save preparation mutates map/cache state. The existing atomic v5 writer is limited to disposable candidate slots.

The new detached [Party stage](../src/Engine/MM6/MM6OutdoorBorderPrecommit.h) copies the live MM6 Party for a cardinal geometric candidate and clamps only the copy's X/Y coordinates to the original border bounds. It refuses invalid routes, disallowed Party flags, flying and unverified corners. It always reports that a checkpoint cannot be written: archive/terrain agreement, complete active and visited-map owners and a durable write result remain missing. The rebuilt integration unit target passed all 11 focused border checks, including three new precommit cases and eight existing route/eligibility controls.

The [later-outdoor writer](../test/Bin/GameTest/GameTests_MM6V5LaterOutdoorCandidate.cpp) now adds a no-write, in-memory checkpoint probe on a loaded second outdoor map. A synthetic out-of-bounds Party copy becomes an in-bounds old-map copy; source/destination base lookup, exact active-map identity, v5 semantic preflight, live represented owner agreement and unchanged live Party core and disposable slots are checked. Fresh v2 seed, v4 baseline and modified writer each passed 1/1 headlessly. The border candidate was neither written nor reloaded. Terrain safety at the clamped point, visited-map runtime owners and durable-write policy remain open, so the fail-closed checkpoint flag is unchanged.

The next transaction must capture complete old-map and visited-map owners in a detached snapshot, validate destination and MM6 eligibility, and make the disk write's durable result explicit. Only after accepted persistence may it publish the old-map cache, apply travel effects once and schedule the target. Failed capture or write must leave live Party, time, food, UI and map state unchanged. A post-rename sync error needs an explicit recovery policy because the new primary may already be present.

The current v5 schema has one active-map runtime set. Saving after entering a later map can discard prior-map actor/sprite/decoration runtime unless a visited-map owner or exact native projection retains it. A fresh-child cycle must reload both the target and old-map checkpoint and verify all owner families, plus an in-session return. Invalid/cancelled/flying/corner cases must perform no write. This remains a design audit: no production MM6 travel save or gameplay admission exists.

The original confirmation trace also establishes conditional rest: any food triggers full rest/heal, insufficient food marks all four characters Weak and adds one no-rest day before food is floored at zero, and zero food skips healing. It refreshes existing Weak timestamps but plays the reaction only for newly Weak characters. It wraps each crossed coordinate and recomputes target ground height. The inherited foot handler heals even with zero food, does not refresh an existing Weak condition, and lacks MM6 coordinate wrapping. Original rejection and separate cancel both clamp coordinates, close travel UI and leave time, food, conditions and map unchanged; the inherited cancel matches those core effects. The source packet records the full transaction order before any live implementation.

See also: [Outdoor Border Trace](MM6-F4-Outdoor-Border-Trace.md), [V5 Later Outdoor](MM6-F4-V5-Later-Outdoor.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md).
