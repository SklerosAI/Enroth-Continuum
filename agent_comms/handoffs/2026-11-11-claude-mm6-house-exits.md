# MM6 house exits and the NPC 12 room chain handoff (M4 lane X, WIP)

- Owner: Claude Code lane. Branch `claude/mm6-house-exits` from `master` `f8f10092a`, in the `mm7-load-admission` checkout. Not pushed.
- Status: research done, no code yet. The worktree has a configured Debug Ninja build with `OpenEnroth_GameTest` built.

## Original behavior (MM6 executable)

House table. The 2DEvents.txt loader at 0x439150 fills 557 records of 48 bytes from 0x52D560, so house `h` sits at 0x52D530 + 48h. Column 20 is the exit picture (+40), column 21 the exit map (+42), column 22 the restriction (+44), all read with atoi into signed words. Text such as "Need Key" reads as 0.

Offering the exit. The portrait builder at 0x43C140 shows the exit only when the picture is nonzero. A positive restriction is a quest bit, and the exit shows only while that bit is set (0x43C17D-0x43C1B1). A zero or negative restriction never hides it. The exit is the last portrait, after the proprietor and the resident NPCs, with its icon from the picture table at 0x4BEFF8 (index 5 and 7 are the ones houses use). In the portrait list its label is the target map's name (0x4982ED). OpenEnroth's inherited block in `prepareHouse` tests the bit the other way round.

Confirming. Clicking the exit portrait (0x4998A0) opens a confirmation view: Cancel (message 113, key N), and Yes, the portrait and the room picture (message 191, keys Y and Space), labelled with global.txt 411 and the map name. The view draws the map name as the title and trans.txt row `map` as the text, or 411 with the map name when the row is empty (0x497240). Rows 47, 49 and 50 are empty.

Moving. Message 191 (case at 0x42EFE2) plays sound 75, autosaves unconditionally through the save routine at 0x44F320 with the autosave flag (0x42F031), sets the next map from mapstats, sets the change-location state, and for a negative restriction `r` sets the arrival to entry |r|-1 of the x, y, z and yaw tables at 0x4BE3B8, 0x4BE3D0, 0x4BE3E8 and 0x4BE400, with pitch and fall speed 0 (0x42F091-0x42F0EB). Zero arrival fields keep the map's own start, as the load at 0x43FBC3 skips them. A restriction of 0 or above arrives at the map's party start. Then it resets the dialogue (0x4A4AA0) and posts message 113.

Houses that use it. 165 to map 49 with bit 167, 170 to map 50 with bit 169, both picture 5. The six outc2 houses 286, 290, 316, 323, 331 and 528 go to map 47 with picture 7 and restrictions -2, -5, -6, -1, -4 and -3, so each picks its own sewer arrival. Houses 153-203 have text in the picture column and so no exit. Scripts enter 165 from outc2:49 and outc2:50, 170 from oracle:3, and the six sewer houses from outc2:59, 63, 89, 96, 104 and 116.

Room chain. The house-entry routine at 0x43C300 checks, for house 168 only, whether any character carries item 461 (0x43C3A5). If one does, the room plays a non-looping movie (name at 0x4BF5E8) instead of its looping room video (0x43C6E4). The movie updater at 0x4A6113 watches houses 165, 168 and 553. When the movie has closed it re-enters 168 as 553 and 553 as 554 (0x4A617F-0x4A619B). MoveNPC (event case at 0x43CD61) clears the loop flag of the room movie while the party is in 553 (0x43CE8E), so the room video plays out and the party lands in 554. NPC 12 lives in 553 and its topic is global:30, whose step 11 is MoveNPC 12 to house 0 after setting bit 177. outd3:42 enters 168 while bit 177 is clear and 554 once it is set. Item 461 comes from global:27 step 9. No shipped MM6 script enters 553 directly. Every .evt in the MM6 icons.lod was scanned for SpeakInHouse.

## Planned design (not started)

New `src/Engine/MM6/MM6HouseExit.h` with the exit plan (picture, map, restriction polarity, arrival table with `@offset`). `MM6HouseServiceData` keeps columns 20-22. `GUIWindow_MM6House` adds the exit as the last portrait, a confirmation view, and on Yes calls `startMapTransition`, which runs `mm6::leaveMap` and the autosave. The room chain hooks the house movie's end in MM6 code. Headless tests need a check of whether the movie end fires without playback.

## Open

Everything in the plan, the tests and all checks. Whether the confirmation buttons should follow the original's y=313 row or the current MM6 house layout at 445.

Privacy: addresses, IDs and map stems only.
