---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, mm7, mm8, status]
---

# Campaign Status

The campaigns have distinct readiness levels. This page is current through the 2026-11-15 MM7 gameplay-harness checkpoint; it is not a claim that every title is fully audited.

The owner's [current delivery order](Current-Delivery-Order.md) puts MM7 working first, MM6 working second, and the full title audits after both working gates. The next-gate cells below describe title-local gaps, not a competing cross-title priority order.

## Working-gate snapshot — 2026-11-10

| Game | Verified state | Next acceptance gate |
| --- | --- | --- |
| MM7 | The November 9 atomic-save, load-preflight and combat repairs reached integration; the later full headless suite passed 533 tests with two skipped after campaign and scroll work. The Harmondale raid case is active and all six campaign GameTests passed on integrated source. Separate input-driven Castle opening and rebuild tests cover first Butler contact, goblin kill versus survivor, castle exit and hand-in, Stone City rescue request, seven rescue interactions versus one missing, and king rebuild outcome; those two tests plus raid passed 3/3 after integration. The rebuilt-castle fresh-process Save/Load checkpoint passed on integrated source: writer and reader 1/1 each, one-missing rescue case 1/1, and prior disposable writer and reader 1/1 each; the slot digest stayed unchanged. A staged Atlas visibly completed New Game, shop purchase, spell cast, named Save/quit/fresh-process Load with item and gold intact, and Load of a copied older save in a disposable profile. The later fail-closed Atlas profile fix is integrated; the signed recording candidate was installed headlessly after owner authorization, with the prior app retained as a rollback copy. On 2026-11-10 the owner's visible session on the installed candidate passed: title music, NPC voice and combat sound heard, an NPC conversation, a fight, a new save, relaunch from The Atlas and Load, with existing saves unchanged. A separate scavenger Judge/ship test passed on integrated source (1/1) with two ship controls (2/2); it supplies delivery items as fixtures and does not cover their acquisition or a fresh-process checkpoint after departure. | The working gate closed on 2026-11-11: a continuous chain of eight chapter tests runs from New Game to the Light ending through real saves, and the full headless suite passed 540/540. The full [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) follows the MM6 working gate. Full [MM7 audit](../project/MM7_AUDIT_CHECKLIST.md) follows the MM6 working gate. |
| MM6 | A seeded first-map session can Save to and Load from numbered native slots; the full headless gate passed 25 children, including three MM7 controls. Character paperdoll, portrait animation and Quest book slices passed focused and hidden checks. On 2026-11-11 production New Game reached the first map, saved from the menu and reloaded in a fresh process (gate 27/27). A sweep of all 67 maps found the blockers, and fixes for the award-animation crash, the reputation variable, the generated NPC pool and the damage channels are integrated. The first campaign chain now runs C0 (New Game to `save000.ec6`) and C1 (a fresh-process Load through councils, stable pricing, chest pickup and `save001.ec6`) in a marked disposable profile; both chapter-save digests passed. MM6 bows now use the native arrow projectile and hit their target, while map visits now preserve sky/timer state across travel and saves; all three revisit checks pass. | Extend the main-quest chain from C1, then prove the visible owner route. This remains headless campaign evidence, not a completed playable-campaign claim. |
| MM8 | Private data is staged for local detection; the runtime is reserved. | Build and verify an isolated MM8 engine, save route and graphical session before any Play action. |

## MM7 audit continuation — 2026-11-15

### Gameplay and Barrow checkpoint — 2026-11-15

The [MM7 gameplay harness](MM7-Gameplay-Harness.md) now provides bounded
press/hold/release movement, attack-input combat and fresh-process suite
admission. A composed Wine Cellar case walks toward the authored Elder Vampire,
releases the key, damages and kills it through attack input, and has an
inspected hidden OpenGL proof. The narrow MM7 compatibility repair activates
only the exact disabled quest actor while its quest is live; completion
blockers and the other disabled group actors remain closed. Tolberti's complete
Light Blaster reward transaction is asserted.

The same checkpoint covers every Barrow Downs map through 27 authored
transitions, plus the Zokarr meditation-pool promotion and the complete Light
Arch Druid route through Green, Nighon Tunnels, Zokarr and Green's return. The
unreachable Hothfarr/event-178 branch is dead original content. Ranger-to-Hunter,
Druid-to-Great-Druid, Archer-to-Warrior-Mage, Knight-to-Cavalier and
Paladin-to-Crusader first promotions use their real dialogue/event routes;
Great Druid physically activates all three authored Stonehenges, while Crusader includes attack-input
Wromthrax combat and the authored 210-second objective timer. A fresh Light C0-C7 chain passed before the owner asked
to pause repeated known-green testing; later slices should receive focused
checks until the next meaningful integration milestone.

Sorcerer-to-Wizard also follows the full physical golem-component route and
both head outcomes. The focused packet exposed Castle Harmondale event 376
enabling group 6 and dismissing the Golem NPC before Thomas's animation; an
MM7-only guard now requires the authored animation bit. The Wizard tests plus
the existing golem control pass 4/4 without rerunning broader suites.

Cleric-to-Priest completes behavior coverage for all nine first promotions.
The route uses Falk's real house, Tidewater's physical quest-map chest and the
Bracada boat schedule, with exact refusal, reward, item, reputation, topic and
duplicate assertions. Its focused packet plus the adjacent Tidewater control
pass 3/3; broader known-green suites remain intentionally unrereun.

Knight-to-Champion now counts the authored combined Knight plus Lord Arena-win
total rather than only the Knight counter. The failing behavior was reproduced
through Leda's real dialogue for 0+5 and 4+1; the event-scoped repair, exact
promotion transaction, refusal matrix and Arena control pass 5/5.

Wizard-to-Archmage physically retrieves the Breeding Zone Book of Divine
Intervention and retains it through Thomas Grey's exact transaction. The route
reproduced event 195 stranding the book after a close-before-pickup; a guarded
map-variable compatibility seam now reopens the source chest during that map
incarnation while preserving fresh-map duplicate prevention. Focused Archmage
and Lantern quest-chest controls pass 6/6.

Rogue-to-Assassin now follows Seknit's real Dark offer through live combat and
corpse looting against Lady Carmine. Celeste event 378 previously passed an
empty group check and enabled Oracle dagger recovery before the assassination;
an exact target seam now requires actor zero's death. Exact retained-dagger
rewards, refusal and duplicate coverage pass 5/5, with the adjacent Wine Cellar
kill control 1/1.

Great-Druid-to-Warlock now uses the real Tor dialogue, authored Land-to-Dragon-
Caves transition and physical egg sprite. Event 103 previously assigned the
Warlock greeting to Anthony Green rather than Tor; the instruction-scoped
recipient correction, exact rewards, familiar hire and duplicate coverage pass
3/3 with the Great Druid control 1/1.

Inspected hidden frames show coherent Tatalia and Avlee views. Shoals' dense
green fog matches its explicit underwater color and distances. The Breeding
Zone black occluder was a literal-origin capture-placement defect and disappears
at the authored floor-resolved start. Selected private images are kept in the
ignored progress folder under a rolling 100-image cap.

The Light campaign's C6 seam now follows the authored Mount Nighon,
Thunderfist Mountain, Tunnels to Eofol, Land of the Giants and Colony Zod
route instead of teleporting to Xenofex. It receives Archibald's Blaster,
rescues Roland, consumes his key, activates both controls, kills Xenofex,
leaves through the authored exit, uses the return altar, respects Lambent's
5am throne closure and completes Resurecta's hand-in before `save006.mm7`.
The shared transition helper now robustly handles clickable faces, pressure
plates and decorations, including the moved Walls-of-Mist exit. A fresh Light
C0-C7 run, five focused Xenofex/Roland/throne-hour controls and the 157-target
style gate passed; the existing C7 ending accepted the strengthened C6 save.

The separately admitted Dark campaign profile is now established through C7.
It reuses common C0-C2, takes the authored messenger/Deyja/Arbiter route in C3,
then carries Archibald's real assignment through the Breeding Zone proof,
authored Pit exit, Gloaming hand-in and three enterable adviser houses before
writing `save004.mm7`. C5 physically retrieves both temple altar pieces and
Thunderfist's Soul Jars, activates Clanker's authored defense event, kills and
loots Robert's live Celeste actor, and returns the cube for all-character
novice Blaster skill and Kastore's relocation before `save005.mm7`. C6 then
follows the authored Mount Nighon, Thunderfist Mountain and Tunnels to Eofol
route; rescues Roland; consumes his key; activates both Colony Zod controls;
kills Xenofex; returns through the Land of the Giants altar; and completes
Kastore's hand-in before `save006.mm7`. C7 accepts Kastore's final task, opens
the two authored Gloaming faces backed by wetsuit-bearing chests, equips all
four suits through inventory input, walks across Avlee's underwater border,
traverses Shoals and Lincoln, powers the console, physically takes the
Overthruster, exits and completes the Dark ending before `save007.mm7`. A fresh
eight-process C0-C7 run passed with retained predecessors unchanged. Both
Light and Dark campaign chains are now complete headless audit anchors; their
named fixture, ordinary movement/combat and graphical gaps remain audit work.

The `codex/mm7-dark-chain` audit line is integrated on `master`. It carries
complete headless coverage for Norbert Thrush's Parsons' Quill route: Emerald
Island contest completion and departure, the real letter offer, a
missing-quill refusal, the Markham Manor exchange, and the final hand-in with
item, quest, award, reputation, gold and XP assertions. Kerrid's route is also
complete through his real offer, the missing-pipes refusal, the Hall Under the
Hill pressure-plate audience event, all-food consumption, letter-to-pipes
exchange, return and rewards. The observed script grants 2,000 XP per character
at the audience and 5,000 more at Kerrid, while writing the journal award only
to the active character. After reconciliation with the current shared core,
the complete MM7 headless suite passed 567 tests with two expected vendor-mod
skips. The upstream `issue_1666` trace update paired with the earlier
valid-indoor-spawn fix is included; the post-MM6-reconciliation regional
side/promotion selection remains green at 10/10.

The same regional gate now also carries the complete Light-path Season's Stole
contract: Gary Zimm's real offer and missing-item refusal, Hall of the Pit chest
acquisition, physical return, quest/item transitions, 7,500 XP per character
and 7,500 gold. As with Kerrid, the source script writes its journal award only
to the active character. The focused test and the 10-test regional selection
both pass headlessly after integration.

The completed Season's Stole, Kerrid, Parsons, Missing Contestants, Three
Paintings, Three Statuettes, Arcomage Champion and Obelisk puzzle routes now also have
independent production-Save/fresh-process-normal-Load checkpoints. Missing
Contestants follows Markham's real offer, proves the
no-shield refusal, clicks the physical Dragon's Lair floor item, and verifies
the item, quest, 1,000-gold, 1,000-XP-per-character and all-character award
contract. Arcomage Champion reaches Gina's real offer and refusal, records all
thirteen authored tavern results through the same persistence transaction used
by the live game, completes her hand-in and takes the Forge Gauntlets from the
authored reward chest. `Issues.Issue388` separately covers live card play; the
quest fixture does not claim thirteen complete matches. The Obelisk route
executes all fourteen authored discovery bindings,
proves the one-missing and 1 AM refusal states, then opens and empties the real
Evenmorn chest at midnight after its scripted 100,000-gold reward. All sixteen
exact writer/reader tests passed headlessly, with final
quest, item, reward, award, map and route-specific food/reputation state intact
and every slot byte-identical after its reader. Admission rejects a missing
disposable profile and a reader profile without a save. The regional selection
passes 14/14 and the repository style gate 157/157. These checkpoints do not
join the eight quests into one continuous campaign.

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
item acquisition is now continuous for the altar pieces, Soul Jars and Pit
cube; real Elder Vampire combat and the Light Blaster reward assertion remain
open. Xenofex request,
alive/dead map-leave and hand-in are integrated, with travel and actor death
as fixtures in those focused controls; the current C6 chain above supplies the
authored travel, Roland/key/controls and live combat. The Overthruster tests now cover throne request, wetsuit chest
collection, Avlee and Lincoln suit gates, Shoals entry, unpowered pedestal,
console activation, item retrieval and exit. Four new cases and the two
wetsuit/Shoals controls plus two Xenofex cases passed 8/8 together on
integration source. The current C7 chain joins that physical retrieval to
Resurecta's final hand-in and Light ending from the fresh C6 save.
The subsequent integrated final-link test passed with the existing Xenofex and
ending controls 3/3, and with the Overthruster cases 7/7. It proves the real
Xenofex hand-in unlocks the second NPC topic for L9, the request activates
the final task, a no-item hand-in stays in the throne room, and a held-item
Overthruster fixture is consumed for the ending. This remains an independent
control; C7 now provides the linked retrieval and fresh-process campaign proof.
The later Pit cube acquisition test is integrated. It enters from the advisor
request and Pit door, defeats the live carrier with bounded player attack
input, picks and clicks the corpse to loot the cube, then returns for the
advisor hand-in. The new test and existing no-cube control passed 2/2 on
integrated source. Party combat strength and positioning are fixtures;
natural walking through the room remains open, while C5 now supplies the
uninterrupted campaign continuity.
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
