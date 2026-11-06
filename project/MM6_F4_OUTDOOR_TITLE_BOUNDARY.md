# MM6 outdoor title boundary — F4

Status: **source-verified title guard, 2026-11-06**. MM6 border travel and random fog remain unimplemented; no MM6 map was entered to validate the view.

`OutdoorLocation::getTravelDestination` previously indexed MM7's Emerald Island–Shoals destination, travel-day and arrival-point arrays using the current map ID. The array and its Avlee/Shoals underwater-suit exception are MM7-only. The MM6 path now returns no border destination before those reads or party mutations. This prevents a numeric MM6 map-ID collision from selecting an MM7 destination. It also means walking across an MM6 outdoor boundary cannot change maps yet.

`OutdoorLocation::SetFog` previously drew from the shared random generator and indexed an MM7 map-ID fog table for any map ID in the MM7 range. The MM6 path now returns before that draw. The outdoor initializer also avoids MM7's underwater-map classification for MM6. MM6 weather probability and underwater rules remain open; existing map weather initialization and time-based density are separate paths and have not been visually validated.

The outdoor loader's assertion now accepts an in-range MM6 map-table row whose normalized filename ends in `.odm`, while MM7 retains its enum-based outdoor assertion. This checks the route to the outdoor loader, not complete map validity. The [F3-matched map-source validation](MM6_F4_WORLD_SAVE_RUNTIME.md) and [offline world readers](MM6_WORLD_F2_CLOSURE.md) cover other boundaries.

Next: reconcile MM6 edge adjacency, travel days, arrival positions, fog and underwater policy with the accepted original source and retained outdoor maps. Add title-specific route and state scenarios, then check visible travel and weather in F5. Cross-map travel also reaches `startMapTransition`'s autosave, which currently rejects MM6 until the `.ec6` transaction is complete; see the [save checklist](../.llm-wiki/MM6-F4-Save-Transaction-Checklist.md). The three MM7-only event campaign exceptions are now title-guarded in `EvtInterpreter.cpp` as part of the same isolation pass.
