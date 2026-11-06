---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, travel, houses]
---

# MM6 Transport Fields F2 Source

The [transport field packet](../project/MM6_TRANSPORT_FIELDS_F2.md) and [probe](../scripts/mm6_probe_transport_fields.py) inventory A/B/C cells from all 21 retained MM6 stable and boat houses. Thirty-seven cells are populated: 34 have regular schedule-like syntax, one has an unresolved `S` day token, and two use special forms. Exact route strings and source hashes stay in the ignored local report. The scanned guide has nine stable and eleven ship itinerary rows, while the retained table has nine stable and twelve boat houses.

The MM7-oriented house loader does not retain route meaning from these cells, and the transport UI uses inherited MM7 arrays. The table is a strong candidate route source, but original departure days, destinations, costs, coordinates and special behavior remain unproven. F3 must reconcile the original file and executable; F4/F5 must test all title-scoped routes and save/reload.

A later direct guide comparison found 34 of 36 printed durations agree with retained cells and two differ; one special boat has no printed row. These are source discrepancies for F3, not route choices for implementation.

See also: [MM6 Campaign Guide Claims F2 Source](Source-MM6-Campaign-Guide-Claims-F2.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
