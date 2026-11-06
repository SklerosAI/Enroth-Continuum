---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, calendar]
---

# MM6 F4 First Map Month Event

The [implementation packet](../project/MM6_F4_FIRST_MAP_MONTH_EVENT.md) narrows one first-map interactive branch bound to four outdoor faces: raw MM6 EVT variable 226 compares the party's **zero-based** cached month with 6. The [MM6 event editor](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) names this ID `MONTH_OF_YEAR`; [MMExtension](https://grayface.github.io/mm/ext/ref/) calls it `MonthIs`. A retained sweep found twelve operands covering 0–11, and the F3-hash-matched original executable compares a modulo-12 cached month word with the operand. This supersedes the first one-based helper interpretation.

MM6-only Compare and dialogue-topic Compare use a checked party-calendar helper over `Party::uCurrentMonth`. Bad ID, operand or cache throws before any event side effect. Writes and other unresolved high IDs remain guarded; MM7 retains its inherited route. The corrected linked unit tests passed 5/5 across the month and flight suites; the headless interpreter tests passed 2/2 with zero on-screen windows or focus change, including a zero-month boundary and invalid operand 12. A real click remains pending. The first-map flying timer uses raw ID 222; its [read-only comparison](MM6-F4-First-Map-Flying-Event.md) has a separate implementation, while the spell effect remains unverified.

See also: [first-map preflight](MM6-F4-First-Map-Preflight.md), [day of year](MM6-F4-Day-Of-Year.md), [F4 offline seams](MM6-F4-Offline-Seams.md).
