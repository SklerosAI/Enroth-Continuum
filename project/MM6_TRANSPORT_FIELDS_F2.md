# MM6 transport-house field inventory (F2)

Status: **retained-table syntax and current-consumer audit, not a proven travel schedule** (2026-11-05). This packet extends [the campaign guide-claim audit](MM6_CAMPAIGN_GUIDE_CLAIMS_F2.md). It reads the already retained, ignored MM6 `2devents.txt` copy, not the original archive, and does not run MM6.

## Reproducible evidence

`scripts/mm6_probe_transport_fields.py` selects all transport-typed rows from the retained `2devents.txt`, preserves their exact A/B/C cells in ignored `local-game-data/mm6/research-2026-11-05/transport-fields-probe.json`, and fingerprints the table plus current [house loader](../src/Engine/Tables/HouseTable.cpp) and [transport UI](../src/GUI/UI/Houses/Transport.cpp). Run it only with an ignored reference and JSON output path. The probe rejects a changed transport-house count before writing a report.

| Retained observation | Count | Meaning and limit |
| --- | ---: | --- |
| Transport-typed houses | 21 | Nine stable and twelve boat rows. This is a table count, not a playable-route count. |
| Nonempty A/B/C cells | 37 | 21 in A, eight in B, eight in C. The field names are inherited column headers, not decoded route types. |
| Schedule-shaped cells | 34 | A destination-like prefix, recognized day tokens and trailing integer. Syntax and selected guide agreement make these route candidates; original behavior is unverified. |
| Schedule-shaped cell with an unknown day token | 1 | The day token `S` must be resolved against the printed guide and original runtime; do not silently treat it as Saturday or Sunday. |
| Other cells | 2 | One compact destination/integer form and one free-destination instruction. Preserve both as explicit cases; neither should be coerced into the regular schedule parser. |

The owner-retained Prima guide's scanned itinerary pages show nine stable and eleven ship rows. The retained table has one additional boat house. A guide omission, edition difference, special service or runtime-only route are all possible; the extra row must not be dropped. The [campaign packet](MM6_CAMPAIGN_GUIDE_CLAIMS_F2.md) records selected guide/table alignment and the still-open quest/map joins.

## Current engine collision

The MM7-oriented `HouseTable.cpp::initializeHouses` comments call A a price multiplier, B empty and C an item-generation interval. For MM6, it forces the A float to zero and attempts numeric parsing of the textual C cell; it does not retain any A/B/C route descriptor. `Transport.cpp` instead indexes static MM7 transport route and schedule arrays by house ID. Six MM6 stable IDs fall outside that inherited route-array range, as recorded by [the campaign route probe](../scripts/mm6_probe_campaign_routes.py). A house record loading or a route-looking cell does not establish a safe, working MM6 travel option.

## F3/F4/F5 contract

At the **single read-only F3 original-installation preflight**, compare the retained table hash, all 21 transport rows and 37 active cells against the original file; inspect original executable route constants or a lawful local reference for the day-token, coordinate, price, duration and special-route meanings. Record exact discrepancies privately. If runtime meaning remains unavailable, carry each cell as an explicit unknown with a test case rather than inventing a route.

For F4, implement a title-scoped MM6 transport descriptor keyed by checked house ID, with validated destination map/coordinate, departure predicate, travel time, cost, story gate and a separate policy for both special cells. Preserve the MM7 arrays and profile. Offline fixtures must cover all 21 houses and every populated option, including out-of-range inherited IDs, a closed day, insufficient gold, cancel, route selection and save-state changes. F5 must inspect the visible house menu, departure, arrival, clock/gold, map and save/quit/relaunch/load for each behavior family. This packet does not close F2 or authorize F3 by itself.
