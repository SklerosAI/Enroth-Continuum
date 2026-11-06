# MM6 transport house menu selector

Status: F4 pure menu data join, 2026-11-06. MM6 house entry and transport departure remain guarded. No travel was performed.

The accepted F3 private transport manifest has 21 houses with three slots each. The retained `2devents.txt` has exactly 21 stable/boat service rows, and their IDs match the manifest house set exactly. All 21 have a validated room descriptor with a file-backed MM6 movie name. This join is based on parsed type labels and the owner-only room/transport manifests; the MM7 contiguous stable/boat ID ranges are not used.

`mm6::selectTransportMenu` takes a decoded MM6 house service table, transport rules, a house ID and a weekday index. It requires a transport service family, a usable MM6 room, a matching manifest entry and weekday 0–6. It returns three optional copies of the already checked F3 route descriptors. Blank, special and ambiguous slots, routes absent on that weekday, and unproved quest-bit-gated routes remain empty. The selector has no access to party gold, time, position, SaveLoad, UI windows, media or sound. It does not map the original one-based Games LOD position to a destination until the guarded caller performs that separate check.

A private-backed native test checks all 21 matched houses against the 536 nontransport rows, all seven weekday indices and all three slots, rejects foreign house/day inputs and missing room/manifest joins, and verifies that every exposed route has no unresolved quest gate. The arm64 MM6/MM7/unit targets built, and the seven focused house/transport tests passed without skips.

The current MM6 transport window can render candidate choices from `TransportRules`, but the global `enterHouse` guard makes it unreachable and selection still reports travel unavailable. Wiring this selector into a future MM6 house window will require a proven game-day-to-weekday conversion, fare and quest policy, room movie/audio behavior, destination/arrival checks and a complete `.ec6` autosave transaction. F5 must observe the visible choices and full travel effects for stable and boat families.
