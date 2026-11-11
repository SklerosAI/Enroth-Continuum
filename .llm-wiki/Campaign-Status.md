---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, mm7, mm8, status]
---

# Campaign Status

The campaigns have distinct readiness levels. This 2026-11-10 snapshot supersedes the historical 2026-11-07 table below; it is not a claim that every title is playable.

The owner's [current delivery order](Current-Delivery-Order.md) puts MM7 working first, MM6 working second, and the full title audits after both working gates. The next-gate cells below describe title-local gaps, not a competing cross-title priority order.

## Current snapshot — 2026-11-10

| Game | Verified state | Next acceptance gate |
| --- | --- | --- |
| MM7 | The November 9 atomic-save, load-preflight and combat repairs reached integration; the later full headless suite passed 533 tests with two skipped after campaign and scroll work. The Harmondale raid case is active and all six campaign GameTests passed on integrated source. Separate input-driven Castle opening and rebuild tests cover first Butler contact, goblin kill versus survivor, castle exit and hand-in, Stone City rescue request, seven rescue interactions versus one missing, and king rebuild outcome; those two tests plus raid passed 3/3 after integration. The rebuilt-castle fresh-process Save/Load checkpoint passed on integrated source: writer and reader 1/1 each, one-missing rescue case 1/1, and prior disposable writer and reader 1/1 each; the slot digest stayed unchanged. A staged Atlas visibly completed New Game, shop purchase, spell cast, named Save/quit/fresh-process Load with item and gold intact, and Load of a copied older save in a disposable profile. The later fail-closed Atlas profile fix is integrated; the signed recording candidate was installed headlessly after owner authorization, with the prior app retained as a rollback copy. On 2026-11-10 the owner's visible session on the installed candidate passed: title music, NPC voice and combat sound heard, an NPC conversation, a fight, a new save, relaunch from The Atlas and Load, with existing saves unchanged. A separate scavenger Judge/ship test passed on integrated source (1/1) with two ship controls (2/2); it supplies delivery items as fixtures and does not cover their acquisition or a fresh-process checkpoint after departure. | The working gate closed on 2026-11-11: a continuous chain of eight chapter tests runs from New Game to the Light ending through real saves, and the full headless suite passed 540/540. The full [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) follows the MM6 working gate. Full [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) follows the MM6 working gate. |
| MM6 | A seeded first-map session can Save to and Load from numbered native slots; the full headless gate passed 25 children, including three MM7 controls. Character paperdoll, portrait animation and Quest book slices passed focused and hidden checks. On 2026-11-11 production New Game reached the first map, saved from the menu and reloaded in a fresh process (gate 27/27). A sweep of all 67 maps found the blockers, and fixes for the award-animation crash, the reputation variable, the generated NPC pool and the damage channels are integrated. The first campaign chain now runs C0 (New Game to `save000.ec6`) and C1 (a fresh-process Load through councils, stable pricing, chest pickup and `save001.ec6`) in a marked disposable profile; both chapter-save digests passed. | Extend the main-quest chain from C1, then prove the visible owner route. This remains headless campaign evidence, not a completed playable-campaign claim. |
| MM8 | Private data is staged for local detection; the runtime is reserved. | Build and verify an isolated MM8 engine, save route and graphical session before any Play action. |

See the [November 9 progress table](Progress-2026-11-09.md) and [MM6 resume handoff](../project/MM6_RESUME_HANDOFF.md) for exact checks and next source paths. Raw captures and game data stay local.

A [passive MM7 playtest recorder](MM7-Playtest-Recording.md) is integrated in the
installed, signed Atlas after the owner authorized a headless reinstall. Its
script, bundle and installed source hashes passed headless checks. The tracked
revision uses a bounded two-minute rolling video buffer and an intentional
clip hotkey; live capture, external-drive behavior and installed-app launch
remain unverified.
This tooling does not advance campaign
playability by itself.

The MM7 item-, actor- and character-target scroll cancellation fixes are
integrated. They return the pending scroll on Escape and still consume it
after a successful target pick. Focused scroll, spell and Light controls
passed 16/16 on integration source, followed by a fresh-process disposable
Save/Load pair 1/1 each with unchanged slot bytes. Other targeting exits
remain in the audit queue.
See [the scroll check](MM7-Scroll-Cancel.md).

The later input-driven Light city and Walls of Mist positive quest route and
the dawn closed-throne control are integrated. The two new tests passed 2/2;
with the arbiter and related issue controls, the integrated focused run passed
4/4. The route reaches the daytime throne, completes the three pedestal
interactions, exits and hands in the quest. The group-1 kill check is a
failure branch: a qualifying kill skips the quest-pass bit, while the no-kill
path sets it. A fresh-entry probe and local base/delta check found no group-1
actor, spawn point, scripted summon or remap. The negative branch is
unreachable in the tested local data; no runtime fix is justified, and the
successful route is unaffected. The later Light chapter checkpoint passed on
integrated source: production Save writer and fresh-process normal Load reader
1/1 each, persistent Light/Walls/guild bits intact and slot digest unchanged.
The follow-up NPC seam check established that the throne hand-in clears its
first topic before Save. NPC79–83 house and relevant topic fields persist
through fresh-process Load and two subsequent frames. The revised Light route
and advisor controls passed 5/5, with writer and reader 1/1 each.
The three advisor hand-in checks and assassination request/cube hand-in are
also integrated. The focused combined Light run passed 7/7. These tests use
fixture quest items and bits around input-driven dialogue and doors. Advisor
item acquisition and uninterrupted chapter continuity remain open; a separate
Pit test below covers combat and cube loot. Xenofex request,
alive/dead map-leave and hand-in are integrated, with travel and actor death
as fixtures. The Overthruster tests now cover throne request, wetsuit chest
collection, Avlee and Lincoln suit gates, Shoals entry, unpowered pedestal,
console activation, item retrieval and exit. Four new cases and the two
wetsuit/Shoals controls plus two Xenofex cases passed 8/8 together on
integration source. An uninterrupted chapter route and L15 with the actual
retrieved item remain open; later focused checkpoints cover adjacent slices.
The subsequent integrated final-link test passed with the existing Xenofex and
ending controls 3/3, and with the Overthruster cases 7/7. It proves the real
Xenofex hand-in unlocks the second NPC topic for L9, the request activates
the final task, a no-item hand-in stays in the throne room, and a held-item
Overthruster fixture is consumed for the ending. It does not yet link the
retrieved item from Lincoln to that hand-in or prove a fresh-process reload.
The later Pit cube acquisition test is integrated. It enters from the advisor
request and Pit door, defeats the live carrier with bounded player attack
input, picks and clicks the corpse to loot the cube, then returns for the
advisor hand-in. The new test and existing no-cube control passed 2/2 on
integrated source. Party combat strength and positioning are fixtures;
natural walking through the room and uninterrupted campaign continuity
remain open.
The later integrated Pit-to-Xenofex seam uses input-driven Walls, altar and
Tolberti hand-ins to produce NPC80's throne house and Xenofex topic without
assigning those fields. A marked disposable production Save writer and
separate normal-Load reader passed 1/1 each with unchanged slot bytes. The
reader selected the surviving second-slot topic through the UI; the in-memory
seam and Xenofex controls passed 3/3. Earlier quest outcomes, altar items and
cube are fixtures, so continuous New Game-to-ending coverage remains open.
The later Overthruster checkpoint is also integrated. Production Save after
input-driven Lincoln retrieval and Shoals exit, followed by normal Load in a
separate fresh headless process, passed 1/1 each. The loaded state kept the
held Overthruster, Light/retrieval bits, four equipped wetsuits and unchanged
slot bytes. Earlier chapter prerequisites are fixtures, so this does not yet
prove uninterrupted campaign continuity or installed-app Save/Load.

## Historical snapshot — 2026-11-07

The following table is retained as a dated record. Its claims that normal MM6 Save/Load remained closed were superseded by the November 9 first-map integration.

| Game | Observed state | Remaining evidence |
| --- | --- | --- |
| MM7 | Native arm64 Atlas launch, Emerald Island graphical view, save/reload, and private portrait and armor AC override checks passed. The separate MM7 baseline was also checked with an existing save. A new staged helper and Atlas built both from a clean tracked-source extraction and a fresh public SklerosAI clone under network denial, and focused headless MM7/Russian-text checks passed. A separately rebuilt hidden OpenGL GameTest rendered one first-map world/HUD frame with no visible window or focus change. A local-only headless GameTest saved to an isolated disk profile and reloaded it through the normal Load menu in a fresh process; the staged candidate has not had visible save/reload acceptance. | Full campaign, quest, map, ending, and broader mod coverage remain to be audited. |
| MM6 | F2 coverage and the [F3 source-data gate](../project/MM6_F3_GATE_REVIEW.md) are complete. A disposable seven-process v5 first-map cycle passed headlessly after live capture extraction. All 15 outdoor source pairs decode; the second outdoor map loads and produces a separate v5 test candidate. Three distinct five-child disposable indoor v6 cycles now pass bounded real-loop turn/walk, post-input save and fresh-child replay; production gameplay admission remains closed. A detached border Party checkpoint and second-outdoor no-write candidate pass focused headless checks. A versioned outdoor v6 visited-map section passed a test-only atomic disk write and fresh-child read in a five-process disposable chain. The reader kept the visited owner detached; a source-backed copied-Party return arrival passed terrain checks, and an opt-in normal-transition probe matched one loaded target floor before aborting to Main Menu. Neither entered a playable returned map. No production checkpoint is written. The proven first-map v5 post-world owner check now runs from an MM6-only read-only engine service; its seven-child disposable replay chain still passes, while the normal save/load menu remains closed. A test-only first-map v5 atomic outcome/preflight policy now handles an unconfirmed post-rename result and rejects wrong-map or foreign-version slots without repair. The first-map v5 source-backed load preflight now lives in an MM6-only detached engine service and still passes the seven-child replay chain. A test-only post-world refusal now aborts to Main Menu before input, while pre-world global publication and rollback remain unproved. A disposable first-map writer now stages exact live owners and classifies atomic outcomes; both reader fixtures construct replay scopes before global assignment. The combined seven-child chain passes. A pure prepublication check now validates event and cursor/shop joins before global assignment; a test-only paused MM6 first-map Save-menu writer now produces a disposable v5 candidate that passes an independent fresh-child replay. Main Menu Quick Load now refuses ordinary MM6 requests even when an `.ec6` slot name exists; the test-only override and MM7 route remain intact. An armed test-only load scope now abandons partial-assignment and pre-event failures to Main Menu before input, then a second full first-map replay reaches exact post-world agreement and bounded input; it does not roll back published globals. An all-source indoor codec sweep passes; the risk-ranked case retains exact signed source bytes but clears this ineligible actor's runtime NPC marker after the original MM6 map-load rule. The earlier pre-event follower refusal is transient. Eligible StreetNPC generation and real dialogue remain open. Active skill Compare/Set and a fail-closed manual/quick-save guard are integrated. | Production MM6 SaveLoad, general indoor gameplay and later-map replay, campaign behavior, graphical F5 and Atlas Play remain open. The disposable cycles are not a playable campaign. |
| MM8 | Private data staged and detected by Atlas. | No adapted MM8 engine or Play action yet. |

The status above is an evidence summary, not a full compatibility claim. A later MM6 first-map checkpoint adds a native numbered-slot admission policy (focused units 2/2) and an Engine-owned staged graph through exact chest handoff. Its seven-child disposable replay, two full-retry fault children and MM7 control passed again. The native slot is not connected to production Save UI, and event/actor/NPC/RNG/post-world ownership still needs extraction from test hooks. For exact counts, parser observations, and known gaps, read the current [local status](../project/LOCAL_STATUS.md) and title-specific implementation documents. The Atlas should expose each Play action only after that title's runnable session and save/load checks.

The [MM7 private portrait mod](MM7-Private-Portrait-Mod.md) has a source-linked mood resolver and Atlas controls. Its focused tests and hidden HUD capture pass, but the new staged bundle has not passed visible interaction or save/reload acceptance and is not installed.
The integration tree also has a focused [MM7 level-seven artifact fix](MM7-Level-Seven-Artifacts.md); its picker and indoor payload tests pass, while real pickup and installed-app acceptance remain open.

Source: [project/LOCAL_STATUS.md](../project/LOCAL_STATUS.md), [project/MM6_F3_GATE_REVIEW.md](../project/MM6_F3_GATE_REVIEW.md), [project/MM7_AUDIT_CHECKLIST.md](../project/MM7_AUDIT_CHECKLIST.md), reviewed 2026-11-07.

See also: [Runtime Isolation](Runtime-Isolation.md), [MM7 Offscreen Frame Capture](MM7-Offscreen-Frame-Capture.md), [MM7 Level-Seven Artifacts](MM7-Level-Seven-Artifacts.md), [Project Overview](Project-Overview.md), [Local Status Source](Source-Local-Status.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [MM6 F4 Native First Map Slot Policy](MM6-F4-Native-First-Map-Slot-Policy.md), [MM6 F4 EC6 V5 Sprite Runtime](MM6-F4-EC6-V5-Sprite-Runtime.md), [Agent Handoff](Agent-Handoff.md), [Open Questions](Open-Questions.md).
