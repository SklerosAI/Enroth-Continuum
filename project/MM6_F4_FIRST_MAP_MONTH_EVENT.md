# MM6 first-map month event

Status: bounded F4 event comparison, 2026-11-06. A real click and its visible outcome remain unobserved.

The retained first outdoor EVT has one reachable interaction that starts by comparing raw variable ID 226 against month 6. The first outdoor map has four faces bound to that event ID, so it is an actual map interaction rather than an orphan script. The false branch reports status and exits; the true branch continues to existing quest-bit and character-stat operations. A separate timer checks raw ID 222 (party flying) only after a quest-bit condition; that variable remains guarded. Other first-map blockers, including cross-map saving and chest interaction, are outside this seam.

The [MM6 event editor's variable table](https://github.com/angeld29/mm8leveleditor/blob/master/src/org/gamenet/application/mm8leveleditor/data/mm6/fileFormat/EventFormatMM6.java) labels ID 226 `MONTH_OF_YEAR`. [MMExtension's event-variable reference](https://grayface.github.io/mm/ext/ref/) names it `MonthIs`. The existing runtime calendar uses one-based months 1–12 and a 28-day month. These sources support an equality comparison for this observed Compare operand. They do not prove all original event UI behavior.

`mm6::compareMonthOfYear` checks ID 226, one-based month range and nonnegative party time, then compares with `Time::toCivilTime().month`. Only MM6 Compare and dialogue-topic Compare dispatch through it, once for the party. The title check leaves MM7's inherited executor intact. MM6 Add, Subtract and Set for ID 226 still fail through the existing title-specific guard; all other unresolved high IDs remain guarded.

Focused offline tests cover the synthetic MM6 record shape, the first and sixth months, adjacent false branches, year rollover and malformed inputs. The integrated arm64 UnitTest passed 3/3. A separate headless GameTest now executes the parsed synthetic Compare through `EvtInterpreter::executeInstruction` with scoped party time, checking the true and false branches without loading a world. Its linked run and a real event click remain pending. No proprietary EVT bytes or private asset index are included here.

See also: [variable dispatch](MM6_VARIABLE_DISPATCH_F2.md), [first-map preflight](MM6_F4_FIRST_MAP_PREFLIGHT.md), [day-of-year comparison](MM6_F4_DAY_OF_YEAR.md).
