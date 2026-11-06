---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, flying]
---

# MM6 F4 First Map Flying Event

The [implementation packet](../project/MM6_F4_FIRST_MAP_FLYING_EVENT.md) records an original-executable-backed read-only comparison for the first-map timer's raw MM6 EVT variable 222. Public [MM6 editor](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) and [MMExtension](https://grayface.github.io/mm/ext/ref/) labels agree on party flight. The locally read, F3-hash-matched executable's Compare branch requires both a flight flag and active buff time and ignores the numeric operand.

MM6 ordinary and dialogue-topic Compare now read `Party::bFlying` and the Fly buff once; they do not mutate them. All writes to this high ID remain guarded, and MM7 retains its existing numeric-ID route. Pure and synthetic headless tests are staged; linked execution and actual timer/spell observation remain pending. This comparison alone does not establish MM6 Fly spell, timer cadence or save/reload behavior.

See also: [first-map month event](MM6-F4-First-Map-Month-Event.md), [first-map preflight](MM6-F4-First-Map-Preflight.md), [F4 offline seams](MM6-F4-Offline-Seams.md).
