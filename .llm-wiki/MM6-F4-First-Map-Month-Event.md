---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, calendar]
---

# MM6 F4 First Map Month Event

The [implementation packet](../project/MM6_F4_FIRST_MAP_MONTH_EVENT.md) narrows one first-map interactive branch bound to four outdoor faces: raw MM6 EVT variable 226 compares the party's one-based month with 6. The [MM6 event editor](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) names this ID `MONTH_OF_YEAR`; [MMExtension](https://grayface.github.io/mm/ext/ref/) calls the variable `MonthIs`. This supports equality, subject to F5 observation.

MM6-only Compare and dialogue-topic Compare use a checked party-calendar helper. Bad ID, month or time throws before any event side effect. Writes and all other unresolved high IDs remain guarded; MM7 retains its inherited route. The separate first-map flying timer uses raw ID 222 and remains a conditional blocker. Offline tests cover a synthetic record, month boundaries, year rollover and invalid operands. Linked unit and headless click evidence remain pending integration.

See also: [first-map preflight](MM6-F4-First-Map-Preflight.md), [day of year](MM6-F4-Day-Of-Year.md), [F4 offline seams](MM6-F4-Offline-Seams.md).
