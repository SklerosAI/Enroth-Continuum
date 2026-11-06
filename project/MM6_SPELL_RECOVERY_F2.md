# MM6 spell recovery: primary-source F2 ledger

Status: **guide categories indexed for all 99 spells; exact original delay table remains open** (2026-11-05). This packet follows the [spell guide and C++ consumer audit](MM6_SPELL_GUIDE_CLAIMS_F2.md). It used the already retained owner manual and Prima guide, GrayFace's MMExtension source, and GrayFace's patch notes. It did not read an installed game or executable, launch MM6, or substitute same-ID MM7 recovery values. The per-ID evidence and page locators are in ignored `local-game-data/mm6/research-2026-11-05/spell-recovery-guide-ledger-private.json`.

## What the primary sources establish

The [Prima guide's spell introduction, printed p. 90 / PDF p. 98](https://www.mocagh.org/nwc/mm6-hintbook.pdf) gives approximate recovery bands: **Very Fast <69; Fast 70–99; Average 100–119; Slow 120–179; Very Slow ≥180**. The printed key leaves 69 unassigned. It also uses **Extremely Slow** for one spell without defining that term numerically. The retained guide text plus direct page-image checks of ambiguous entries now assign one printed baseline category to **all 99 IDs**: 3 Very Fast, 8 Fast, 24 Average, 51 Slow, 12 Very Slow, and 1 undefined Extremely Slow. These are guide claims and broad ranges, not exact original executable constants or a conversion to the current engine's ticks.

The guide explicitly supplies **84 Expert/Master category cells** and **two numeric Expert/Master cells** for one spell. It states a **base minus skill** casting-recovery relationship for six Normal spells, but leaves the numeric base and clamp unspecified. The other rank clauses do not explicitly change casting recovery. Treating silence as “same delay as the previous rank” would be an inference, so the private ledger marks it separately and does not fill a numeric value. The [owner manual, PDF p. 51](https://www.mocagh.org/nwc/mm6-manual.pdf) describes First Aid's recovery qualitatively and as skill-reduced; its adjective differs from the Prima baseline category. That tension needs F3 adjudication.

GrayFace's pinned [MMExtension MM6 `SpellInfo` definition](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/01%20common%20structs.lua#L1749-L1765) defines three signed 16-bit delay fields at structure offsets 6, 8 and 10 for Normal, Expert and Master, and labels them hard-coded. Its [`Game.Spells` array definition](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/00%20structs.lua#L569) exposes the MM6 spell-indexed structure. This is a primary source for **where and how to read** the values, not for the 99 numeric triples. GrayFace's [MM6 patch notes](https://grayface.github.io/mm/MM6PatchReadMe/) also identify recovery-delay ordering in Town Portal and Lloyd's Beacon autosave flows, without supplying their constants.

## Closure counts and F3 target

| Evidence level | Coverage | Meaning |
| --- | ---: | --- |
| Printed baseline category | 99/99 | Approximate guide claim; 98 use defined bands, 1 is undefined. |
| Explicit Expert/Master category | 84/198 rank cells | Sourced qualitative rank claim. |
| Explicit numeric Expert/Master claim | 2/297 total rank cells | Guide numbers for one spell, still unverified against original execution. |
| Guide-stated Normal skill subtraction | 6/99 | Arithmetic relationship with unknown base and minimum. |
| Original-verified complete delay triples | 0/99 | Requires the planned F3 read. |
| Previously unresolved F2 recovery facets with complete exact triple now | **0/96** | All 96 remain open for numeric implementation; the guide narrows many to bands. |

At F3, record the exact installed executable version and hash privately, then make the planned **single read-only extraction** of all 99 Normal/Expert/Master delay triples. Confirm the structure and ID alignment against MMExtension's MM6 layout, the retained spell identity table, the guide's bands and its two printed numeric cells. Record any disagreement without changing the source claim. Also determine whether skill subtraction, rank overrides, failure/cancellation, scrolls, event/monster casts, and portal/beacon transitions use the table values or additional code paths. The guide does not define the lower clamp or unit conversion.

At F4, implement a title-specific MM6 recovery table and source-backed dynamic rules, preserving MM7's existing table. Offline checks must cover 99 IDs × three ranks, mismatched labels, category boundaries, skill subtraction and failure/transition paths. At F5, observe action readiness in both combat modes and after save/reload for representative spells, including travel spells. **No same-ID inherited MM7 delay is accepted as an MM6 value.**
