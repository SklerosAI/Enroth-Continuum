---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, events, character-state]
---

# MM6 F4 Character Event Bits

The [implementation packet](../project/MM6_F4_CHARACTER_EVENT_BITS.md) covers raw MM6 EVT variable 213: eleven retained Compare and twelve Set uses across seven campaign resources. [MM6 editor](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) and [MMExtension](https://grayface.github.io/mm/ext/ref/) labels identify a character event bit. The F3-hash-matched original executable confirms a zero-based wire bit test/set using an MSB-first mask. Runtime `IndexedBitset<1, 512>` keys are one-based, so the adapter shifts checked operands by one.

MM6 Compare and Set now use the selected character(s) through a bounded title-specific adapter. Set validates before touching any character; Add/Subtract and other high IDs remain guarded. MM7 stays on its inherited path. Unit tests cover bounds, idempotence and serialized masks; a synthetic headless test stages the interpreter branch with scoped restoration. The integrated focused native group passed 27/27 and monitored focused GameTest group 6/6 without focus change or visible window. Live campaign consequence and durable save/reload are still pending.

See also: [F4 offline seams](MM6-F4-Offline-Seams.md), [first-map month event](MM6-F4-First-Map-Month-Event.md), [save remainder](MM6-F4-Save-Remainder.md).
