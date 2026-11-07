---
type: synthesis
up: "[[index_Enroth-Continuum]]"
extends: "[[MM6-F4-Chest-Interaction-Gate]]"
tags: [mm6, chest, transaction, campaign]
---

# MM6 F4 Chest Open Owner Transaction

The opt-in owner transaction joins one untrapped MM6 chest's first placement to its active map sidecar and title-local LCG.

`stageChestOpenOwner` decodes the current indoor or outdoor map delta, requires twenty raw records, and compares the selected live chest's header, supported item fields and full grid with the raw record. A world position is allowed and preserved. It calls the detached, source-backed `stageChestFirstOpenInteraction` in open-only mode, projects the changed record into a runtime chest, patches the one active sidecar record, and returns copies of the candidate owners. `applyChestOpenOwner` publishes the candidate with no-throw vector swaps and a scalar LCG assignment only after every stage succeeds. Reopening an initialized chest leaves the sidecar and LCG unchanged.

The path refuses a foreign title, absent or unready title state, malformed sidecar, stale runtime, invalid chest index, trap, failed icon size lookup, or failed projection without owner mutation. It does not enter `Chest::open`, create a chest window, handle a trap or item pickup, or call inherited MM7 generation. The caller must provide the MM6 `dchest` bytes and a validated icon-size lookup; no normal event caller constructs this binding yet. A normal playable chest still requires the active event entry, UI/pickup and save admission to be proved together.

Linked arm64 synthetic tests passed 3/3 for outdoor first-open/reopen, indoor sidecar patching, position retention, raw sidecar/runtime agreement and rollback for foreign title, trap and stale runtime. They are source checks, not a playable chest or graphical acceptance.

See also: [Chest Interaction Gate](MM6-F4-Chest-Interaction-Gate.md), [Chest Live Owner Adapter](MM6-F4-Chest-Live-Owner-Adapter.md), [Chest Map Load Route](MM6-F4-Chest-Map-Load-Route.md).
