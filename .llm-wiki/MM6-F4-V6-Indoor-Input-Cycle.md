---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Face-Overrides]]", "[[MM6-F4-Save-Transaction-Checklist]]", "[[Campaign-Status]]"]
tags: [mm6, f4, indoor, saves, headless]
---

# MM6 F4 V6 Indoor Input Cycle

A disposable headless indoor v6 candidate now survives one real-loop turn and walk, a separate atomic post-input write, and exact fresh-child replay.

The [project packet](../project/MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md) is the dated verification record. Five separate integration GameTest children passed **1/1 each** on 2026-11-07: v2 seed, v4 baseline, v6 live-session writer, first-input/post-input writer, and final fresh-child reader. An earlier four-child chain with first input but no second write also passed **1/1 each**. Every slot lived in a 0700 owner-only system-temp profile; no private game data or save bytes entered Git.

The v6 writer captures live Party/title/raw and GNPC/RNG/EVRT/IFOV/IART/ISRT/IDRT owners from a genuine post-world indoor state. A first fresh child reconstructs the pre-event BLV/DLV and face base, stages the candidate, publishes the indoor runtime owners before `OnMapReload`, checks the represented owners at the real `Game::gameLoop` post-world boundary, and only then queues one turn and walk. Position and yaw both change over three bounded ticks. A second atomic candidate captures the settled post-input state. Its fresh child replays and recaptures exact represented owners after map events.

The selected fixture has no valid named entrance. Its test-only starting position is derived from native BLV spawn/floor geometry and admitted only with valid floor, body/eye sectors and short forward clearance. Indoor minimap drawing can reveal `_visible_outlines`; the post-input writer checks that new bits are monotonic and backed by live map-outline flags, then patches only the DLV Visibility section. Reset, chest, door and opaque DLV fields retain their strict source checks. This supersedes the older summary that no indoor first input or full live-session Party/title candidate had been checked.

A [second indoor source case](../project/MM6_F4_EC6_V6_INDOOR_INPUT_CYCLE.md#second-indoor-source-case) now passes separate deterministic v2 seed, v4 baseline and v6 live-session writer children. It uncovered and fixed two MM6-only source-consumer gaps: native indoor actors now retain their per-actor resistance bytes, and the renderer skips an empty `DECORATION_NULL` slot. The original first-indoor five-child cycle and a focused MM7 headless control still pass. The second case's fresh-child post-world check refuses **before input** because door motion differs from the retained DLV; the existing v6 owner model does not yet explain or replay it. No second-map post-input save or reload was accepted.

The v6 stage's `canEnterGameLoop()` remains **false**. A one-shot test scope alone permits the bounded input after exact post-world checks. Production MM6 SaveLoad, normal autosave/menu loading, general indoor and campaign play, a graphical indoor view, audio, and Atlas MM6 Play remain open. Earlier indoor [IFOV and runtime owner](MM6-F4-Indoor-Face-Overrides.md) gates and the separate [outdoor v5 cycle](MM6-F4-V5-Later-Outdoor.md) do not change those boundaries.

See also: [Indoor Face Overrides](MM6-F4-Indoor-Face-Overrides.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [V5 Later Outdoor](MM6-F4-V5-Later-Outdoor.md), [Campaign Status](Campaign-Status.md), and [Agent Handoff](Agent-Handoff.md).
