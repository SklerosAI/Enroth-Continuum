# MM6 house type routing

Status: F4 parser boundary, 2026-11-06. This is an offline routing change, not a verified house screen or service.

The retained `2devents.txt` table has 557 house rows. The prior `HouseTable.cpp` parser recognized the inherited MM7 type vocabulary and sent 384 active nonempty MM6 labels to `HOUSE_TYPE_MERCENARY_GUILD`. The resulting `createHouseUI` path selected the mercenary guild screen even for residences, entrances and distinct services. The private source report is `local-game-data/mm6/research-2026-11-05/house-route-probe.json`; its per-house details and hashes stay ignored.

`parseHouseType` now handles the retained MM6 labels by title. Shop and guild labels already recognized by the shared table keep their existing enum values. MM6 service labels such as general store, mercenary guild, thieves guild, council, circus, seer and jail receive explicit type values. Numbered residence labels, including the retained spelling variant, become generic houses. Entrances and other named locations receive an explicit generic or location type; an unrecognized MM6 label becomes `HOUSE_TYPE_INVALID`, which the current UI routes to the generic house screen. The MM7 parser retains its prior fallback.

The refreshed ignored report is format 2 and owner-only (mode 0600). It records 81 distinct labels, 421 table rows that previously took the legacy fallback, and 384 such active rows. It finds zero unclassified labels across all 557 rows and active entries. Native tests cover representative services, entrances, residence variants, unknown MM6 labels, MM7 behavior and the complete retained 557-row source.

This fixes the parser's silent category collision only. The MM6 room movie/background/NPC descriptor table, service logic, house-ID-range helpers, prices, opening hours and live screens are not established. In particular, a correctly typed `General Store` still reaches the generic window until its own service is implemented; `Wagon` and `Temple Ent` remain generic until their exact actions are proven. F5 must verify entrances, shops, guilds, residences, travel, jail and special locations visually and functionally.
