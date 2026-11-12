---
type: source-summary
up: "[[Campaign-Status]]"
related: ["[[Current-Delivery-Order]]", "[[MM7-Disposable-Save-Reload]]"]
tags: [mm7, quests, light, dark, side-quests, promotions, test-contracts]
---

# MM7 Quest Behavior Catalog

This is the source-linked intended-behavior contract for every MM7 main, side,
and promotion quest before a quest is accepted as covered by a GameTest.

## Method and confidence

`C` means two independent public guides agree on the player-facing outcome;
`S` means one useful public guide was found; `O` is a known ambiguity requiring
an original-game observation or a narrow event-script audit before a test can
claim the behavior. Sources describe behavior in our own words; no guide text
is reproduced. "Covered" means a current test reaches the behavior through the
game, not that every travel/combat detail lacks an intentional fixture.

The catalog's scope is every published quest-log objective and quest-bit-backed
quest in MM7: main path, alignment path, side quests, promotions, and the
Obelisk puzzle. It deliberately excludes repeatable services, merchants,
trainers, passive shrines/wells, and generic map loot. This boundary caught two
previous omissions in the side table (the Dark Griffin hunt and Obelisks).

Primary sources: [RPGClassics side quests](https://shrines.rpgclassics.com/pc/mm7/sidequests.shtml),
[RPGClassics first promotions](https://shrines.rpgclassics.com/pc/mm7/promoquests1.shtml),
[RPGClassics Dark promotions](https://shrines.rpgclassics.com/pc/mm7/promoquests2d.shtml),
[RPGClassics Light promotions](https://shrines.rpgclassics.com/pc/mm7/promoquests2l.shtml),
[RPGClassics Obelisks](https://shrines.rpgclassics.com/pc/mm7/obelisks.shtml),
[Mike's RPG Center walkthrough](https://www.mikesrpgcenter.com/mm7/walkthrough.html),
[Mike's side-quest index](https://mikesrpgcenter.com/mm7/sidequests.html),
[DungeonCrawl Classics quest table](https://dungeoncrawl-classics.com/might-and-magic/mm7/mm7-guide/),
[RPGClassics Dark walkthrough](https://shrines.rpgclassics.com/pc/mm7/walk4dark.shtml), and
[RPGClassics finale walkthrough](https://shrines.rpgclassics.com/pc/mm7/walk5.shtml).
The [Prima official strategy guide](https://www.mocagh.org/nwc/mm7-hintbook.pdf)
resolves the war-clock timing below; its claim is corroborated by the
[GOG guide discussion quoting the same pages](https://www.gog.com/forum/might_and_magic_series/mm7_trumpet_timing/page1)
and by the [DungeonCrawl quest table](https://dungeoncrawl-classics.com/might-and-magic/mm7/mm7-guide/).

## Shared main path

| ID | Intended behavior | Current test status | Confidence |
| --- | --- | --- | --- |
| M0 | Give Judge Thomas the six required Emerald Island contest items; Markham awards Harmondale and offers departure. Missing any item refuses completion. | C0 and `MM7ScavengerPath`; item acquisition is separately open. | C |
| M1 | Meet the Butler, clear all Castle Harmondale goblins, leave, and report back; this unlocks the dwarf repair route. | C1 and Castle-path controls, including survivor refusal. | C |
| M2 | Hothfarr gives the elixir route; cure all seven petrified dwarves and report to the king to rebuild Harmondale. | C2 and rebuild controls, including one-missing refusal. | C |
| M3 | Catherine's Loren Steel rescue and Parson's Riverstride-plans request each have a one-month response window. The party may complete the real task, deliver its decoy, or decline; those choices determine the war result unless the trumpet is used. | Only the Light continuation is chained. Real/decoy/ignored action matrix is uncovered. | C |
| M4 | Two months after the war starts, entering Tularean Forest activates the Gryphonheart's Trumpet battle. Give it to Erathia or Avlee to decide the war, or to Judge Grey after a balanced/neutral record to preserve Harmondale independence. Judge Grey dies four months after the war clock starts, then enables the Arbiter selection. | Light C3 plus choice/end smoke tests. Timer, trumpet recipient, neutral and Dark selector controls are absent. | C |
| M5 | Recruit Fairweather for Light or Sleen for Dark and install the candidate at Judge Grey's house. The choice is permanent and changes alignment relations. | Light C3; Dark selector is absent. | C |

## Light main path

| ID | Intended behavior | Current test status | Confidence |
| --- | --- | --- | --- |
| L1 | Gavin Magnus sends the party through Walls of Mist; avoid killing its protected inhabitants, solve the key/pillar route, exit, and report back to join the Light guild. | C4 and focused Walls tests. Local group-ID predicate is unreachable, so retain the no-kill player contract and audit the predicate separately. | C / O for local predicate |
| L2 | Resurecta requires both altar pieces from the Light and Dark temples; return both after their dungeon interactions. | C5 plus item/adviser tests. | C |
| L3 | Crag Hack's Wine Cellar task requires the Elder Vampire to be defeated before return. | `MM7LightItems.WineCellarExitDoesNotReplaceVampireKill` rejects the disabled spawn/exit false positive, then proves the genuine map-leave kill predicate completes the adviser hand-in. | C |
| L4 | Caneghem requires the Case of Soul Jars from Castle Gloaming. | C5 and item/adviser tests. | C |
| L5 | Once the three advisers are complete, Robert requests Tolberti's control cube; kill Tolberti, loot the cube, return it, and gain Blaster training. | C5 and Cube combat/hand-in tests; add reward assertion. | C |
| L6 | Resurecta sends the party after Xenofex in Colony Zod; the normal route includes Roland/key/control access, Xenofex's death, then Resurecta hand-in. | C6 proves combat and hand-in, but teleports past Roland/access route. | C; route gap |
| L7 | Acquire/equip four wetsuits, reach Shoals and Lincoln, restore power, take the Oscillation Overthruster, return to Resurecta, and receive the Light ending. | C7; individual gate controls and ending are present. | C |

## Dark main path

| ID | Intended behavior | Required future contract | Confidence |
| --- | --- | --- | --- |
| D1 | Meet Archibald in Castle Gloaming, complete the Breeding Zone proof, exit, and unlock Dark-side adviser progression/Dark Magic membership. Killing every inhabitant is not the stated completion rule. | `MM7DarkGuild` now covers the real map marker, Archibald assignment, and Archibald hand-in/guild unlock. The durable Dark chain is next. | C |
| D2 | Kastore requires both altar pieces, using the Dark counterpart route and returning them to Kastore. | `MM7DarkGuild.KastoreRequiresBothAltarPieces` reaches Kastore through Archibald's actual unlock and runtime-selected Pit house, then covers assignment, one-piece refusal, two-piece hand-in, and consumption. Physical temple acquisition remains a separate continuous-chain concern. | C |
| D3 | Maximus requires the Soul Jars from Thunderfist Mountain, not the Light-side Gloaming route. | `MM7DarkGuild.MaximusRequiresAndAcceptsSoulJars` reaches the unlocked Pit advisers, verifies assignment and refusal without the case, and its completion/consumption. `ThunderfistChestProvidesDarkSoulJars` discovers the loaded map event/chest association, opens that production chest UI, and transfers the case; it deliberately does not depend on a renderer-sensitive outdoor aim ray. | C |
| D4 | Dark Shade requires Clanker's Laboratory's magical/teleport defenses disabled; it is not a kill-all dungeon objective. | `MM7DarkGuild.ClankerDefensesAreDisabledByMapEvent` proves the map-state source; `DarkShadeRequiresDisabledClankerDefenses` covers Dark Shade's assignment, refusal before that marker, and hand-in. | C |
| D5 | After the three Dark advisers, Tolberti in the Pit sends the party after Robert the Wise in Celeste. Obtain Robert's control cube by killing or stealing it, then return it to Tolberti. Robert drops a Blaster as well as the cube; the hand-in grants every party member novice Blaster skill. Normal Blaster training is thereby enabled, with Dark expert/master/grandmaster trainers in the Pit/Gloaming. | `MM7DarkGuild.TolbertiRequiresAndAcceptsRobertsCube` covers Dark assignment, refusal without a cube, the distinct hand-in/consumption, and novice Blaster skill for all four party members. `CelesteRobertCarriesControlCube` takes the actual Celeste door to the Small House and locates Robert's live cube-and-Blaster source; combat/theft retrieval remains focused work. | C |
| D6 | Kastore's Dark quest requires the Hive/Colony Zod route: rescue Roland, activate the two route controls that open the way, kill Xenofex, then return to Kastore. | `MM7DarkGuild.KastoreRequiresXenofexKill` follows Tolberti's real relocation of Kastore to Gloaming, then covers assignment, refusal before the shared death marker, and hand-in. The Roland/control access route and combat remain focused contracts. | C |
| D7 | After the Xenofex return, use four wetsuits (only Blasters may remain equipped) to reach Lincoln from Avlee, re-equip inside, restore Lincoln power, retrieve the Oscillation Overthruster, and return it to Kastore for the Dark ending. | `MM7DarkGuild.KastoreAssignsDarkOverthrusterAfterXenofex` reaches the final event through actual prior Dark transitions and asserts its Dark active bit. `MM7Campaign.Endings` covers the shared Dark Kastore hand-in/end-state; Lincoln's physical Dark-route acquisition and a continuous Dark ending remain focused contracts. | C |

## Non-branch side quests

| Quest | Intended completion/outcome | Test status | Confidence |
| --- | --- | --- | --- |
| Missing Contestants | Return the Contestant Shield from Emerald Island’s Dragon Cave to Markham; award is 1,000 XP and 1,000 gold. | Uncovered. | C |
| Lantern of Light | Tarin Withern assigns the Lantern recovery; return the physical Lantern from the Barrow II chest to receive 5,000 XP per character, 1,000 gold, and the Harmondale award. | `MM7SideQuests.LanternOfLightRequiresBarrowChest` dynamically finds Tarin, proves acceptance and no-item refusal, loots the event-mapped Barrow II chest through the chest UI, and verifies normal hand-in consumption and rewards. | C |
| Davrik’s Signet Ring | Recover the Bandit Caves ring after accepting Davrik's task, then return it for 5,000 XP per character, 5,000 gold, and the Harmondale award. | `MM7SideQuests.DavriksSignetRingRequiresBanditCavesChest` dynamically finds Davrik and the tavern, proves the empty-handed refusal, loots the quest-gated Bandit Caves chest through its normal UI, and verifies the ring consumption, state transition and rewards. | C |
| Darron’s brother | Find the White Cliff evidence/deck and return to Darron Temper; retain the Arcomage deck, receive 2,000 XP and 750 gold. | `MM7SideQuests.DarronsDeckIsPhysicallyAcquiredAndReturned` reaches Darron's real Harmondale house dialogue, accepts the task, finds the persistent White Cliff Cave deck object, clicks it through the world-item path, then returns it to Darron. It verifies active-state clearance, retained deck, 750 gold and 2,000 XP per character. | C |
| Three statuettes | Place Eagle/Knight/Angel statuettes at their three shrines, then return to Thom Lumbra for 50,000 XP and 50,000 gold. | `MM7SideQuests.AngelStatuetteIsAcquiredFromTheMazeChest` repeat-verifies the physical Angel Statuette acquisition through the normal Maze chest UI. Knight/Eagle acquisition, all three shrine placements, and Thom's return/rewards remain uncovered. | C |
| Kerrid letter | Deliver Kerrid’s letter to the Faerie King, receive pipes, and return them. The audience consumes all party food and grants 2,000 XP per character; Kerrid grants another 5,000 XP per character and 1,000 gold. The script writes the journal award only to the active character. | `MM7SideQuests.KerridLetterRequiresFaerieKingExchange` dynamically finds Kerrid, the Faerie King, and the Hall Under the Hill audience event. It covers the real offer and physical letter, the missing-pipes refusal, all-food consumption, letter-to-pipes exchange, quest-bit transitions, 7,000 total XP per character, 1,000 gold, pipe consumption, and the active-character-only award behavior. | C |
| Norbert’s quill | Deliver Norbert’s letter to Markham, return Parson’s Quill for 5,000 XP and 2,000 gold. | `MM7SideQuests.ParsonsQuillRequiresMarkhamExchange` completes the Emerald contest and ship departure through input, accepts Thrush's letter, proves his visible return topic refuses the missing quill, exchanges the physical letter with relocated Markham in his Tatalia manor, and returns the quill. It verifies item/quest transitions, 2,000 gold, -5 reputation, the Erathia award and 5,000 XP per character. | C |
| Arcomage champion | Win once in each of thirteen taverns, then return to Gina Barnes for 50,000 XP, 100,000 gold, and the chest reward. | Uncovered. | C |
| Three paintings | Return the Angel, Archibald, and Roland paintings to Visconti for 50,000 XP and 50,000 gold. | Uncovered. | C |
| Haldar’s remains | Mazim Dusk assigns recovery of Haldar's physical remains from the Maze; return them for 50,000 XP per character, 35,000 gold, and the Nighon award. | `MM7SideQuests.HaldarsRemainsRequireMazeChest` dynamically finds Mazim, proves acceptance and no-item refusal, loots the event-mapped Maze chest through the chest UI, and verifies normal hand-in consumption and rewards. | C |
| Season’s Stole | Recover the stole from the Hall of the Pit and return it to Gary Zimm for 7,500 XP and 7,500 gold. | `MM7SideQuests.SeasonsStoleIsAcquiredFromHallOfThePitChest` activates the quest, locates the Stole-bearing Hall of the Pit chest in loaded game state, and takes it through normal chest UI. The Gary Zimm return/reward remains uncovered. | C |
| Troglodytes | Clear Stone City’s lower-tunnel troglodytes and report to Spark Burnkindle for 5,000 XP per character, 2,500 gold, -10 reputation, and the lower-city award. The Nighon passage is excluded. | `MM7SideQuests.TroglodytesRequireLowerCityClearance` proves assignment, refusal with one survivor, and the complete-clear state/reward contract. Direct combat remains separate. | C |
| Griffins | On the Dark path, kill every Griffin in both Erathia and Bracada Desert, then report to Seth Drakkson for 10,000 XP and 5,000 gold. | `Issues.Issue2868a` proves the per-map all-Griffin predicate (including the former Gold Golem mismatch); `Issue673d` covers assignment/report completion and reputation. Add XP, gold, award and refusal assertions. | C |
| Malwick's favor/Niles debt | Taking Malwick's Wand of Fireballs on Emerald Island conditionally starts the later Niles chain. Within six weeks of choosing a path, the party receives the Guild message on entering Harmondale; it then has two weeks to visit Niles and two more weeks to return the path-appropriate allied-castle tapestry. | Partial focused coverage; expand into the explicit clock/trigger cases. | C |
| War alternatives | Cover Loren real/decoy/decline, Riverstride real/decoy/decline, the resulting war score, trumpet recipient, and the neutral option independently. | Uncovered matrix. | C |
| Niles/tapestry | Recover the allied-castle tapestry (Lambent on Light, Gloaming on Dark) and return it to Niles. Taking it makes that allied region hostile for one month. Missing either two-week deadline causes Goblins and Swordsmen to attack Harmondale; shops/services close until the invaders are cleared. | Uncovered. | C |
| Obelisk puzzle | Examine all fourteen regional Obelisks, then visit the Evenmorn stone circle between midnight and 1 AM to receive the 100,000-gold treasure chest and its items. | Uncovered. | C |

## Promotions (individual side-quest contracts)

Every promotion test must assert the intended class receives the class change,
an ineligible party member receives only the honorary outcome, and a second-tier
test rejects the wrong alignment or a missing first promotion. First promotions
award 30,000 XP to a qualifying class and 15,000 XP honorary; second promotions
award 80,000 XP qualifying and 40,000 XP honorary. All rows are uncovered
unless stated otherwise.

| Tier | Quest and intended completion | Confidence |
| --- | --- | --- |
| First | Monk → Initiate: accept from Bartholomew Hume, find and use the meditation pool in Zokarr's Tomb in the Dwarven Barrows. The pool remotely completes the quest and grants the promotion (no return visit). | C |
| First | Ranger → Hunter: complete the Faerie King's task. | C |
| First | Druid → Great Druid: activate the three druid monoliths. | C |
| First | Archer → Warrior Mage: use the supplied Worn Belt to sabotage the Red Dwarf Mines lift, then return. | C |
| First | Knight → Cavalier: clear the Haunted Mansion task. | C |
| First | Paladin → Crusader: defeat Wromthrax. | C |
| First | Cleric → Priest: recover the pirate map; its other intended effect is opening the Evenmorn route. | C |
| First | Thief → Rogue: accept William Lasker’s request in the Erathian Sewers, take Markham’s Vase from the Markham Manor fireplace, and return it. `MM7Promotions.MarkhamVaseRequiresPhysicalPickup` proves offer, no-vase refusal, real map pickup, hand-in consumption, promotion/honorary split, XP and gold. | C |
| First | Sorcerer → Wizard: recover and assemble all six golem components; separately cover the good/bad head choice. | C |
| Light second | Initiate → Master: on the Light path, accept from Bartholomew Hume, kill the High Priest of Baa in Avlee's Temple of Baa, collect his cloak, then return to Hume. | C |
| Light second | Ranger → Ranger Lord: return the Heart of Wood. | C |
| Light second | Druid → Arch Druid: retrieve Zokarr IV's skull, return it to his tomb, then report back. | C |
| Light second | Archer → Master Archer: return the Perfect Bow. | C |
| Light second | Knight → Champion: win five Knight Arena bouts. | C |
| Light second | Paladin → Hero: rescue Alice. | C |
| Light second | Cleric → Priest of Light: purify the Sun altar. | C |
| Light second | Thief → Spy: recover the Watchtower counterweight. | C |
| Light second | Sorcerer → Archmage: recover the Book of Divine Intervention from the Breeding Zone and return it. | C |
| Dark second | Monk → Ninja: solve Ashwar Nog'Nogoth's tomb access, clear it, and return to Stephan Sand. | C |
| Dark second | Ranger → Bounty Hunter: turn in 10,000 gold in bounties. | C |
| Dark second | Druid → Warlock: return the dragon egg. | C |
| Dark second | Archer → Sniper: return the Perfect Bow. | C |
| Dark second | Knight → Black Knight: rob the Navan treasury and return to Frederick Org. | C |
| Dark second | Paladin → Villain: capture Alice. | C |
| Dark second | Cleric → Priest of Dark: deface the Sun altar. | C |
| Dark second | Thief → Assassin: kill Lady Carmine, return her dagger to Seknit Undershadow. | C |
| Dark second | Sorcerer → Lich: return the Proving Grounds soul jars. | C |

## Current-test-to-contract map

| Existing test family | Contract it is allowed to claim | Required correction or follow-up |
| --- | --- | --- |
| `MM7CampaignChainC0`, `MM7ScavengerPath` | M0 delivery, refusal when one contest item is missing, Judge and ship progression. | Add separate acquisition checks for all six items. |
| `MM7CampaignChainC1`, `MM7CastlePath` | M1 Butler/goblin/report sequence and survivor refusal. | None before broader side coverage. |
| `MM7CampaignChainC2`, `MM7CastleRebuild` | M2 elixir, seven-dwarf, rebuild sequence and one-missing refusal. | Keep its save boundary. |
| `MM7CampaignChainC3`, `MM7Campaign` | Light Arbiter selection and post-choice house relation. | Add M3–M5 timer, war-score, trumpet/neutral, and Dark selection cases. |
| `MM7CampaignChainC4`, `MM7LightWalls`, `MM7LightItems` | L1 key/pillar path and no-kill player rule. | Audit the unreachable local protected-group predicate. |
| `MM7CampaignChainC5`, `MM7LightItems`, `MM7LightAdvisors`, `MM7PitCubeAcquisition` | L2, L3 death-state plus exit/hand-in, L4, and L5 item/return progression. | Replace the C5 direct death-state fixture with real Elder Vampire combat; assert Blaster training reward. |
| `MM7CampaignChainC6`, `MM7LightXenofex`, `MM7XenofexCombat` | L6 Xenofex combat and Light hand-in. | Add Roland/key/control access route. |
| `MM7CampaignChainC7`, `MM7OverthrusterRoute` | L7 wetsuits, Lincoln, Overthruster, Light ending. | Retain all individual gate controls. |
| Future `MM7DarkCampaignChainC1`–`C7` | D1–D7 through normal, durable save boundaries. | Include recipient, alignment, combat/event and ending assertions named above. |
| `MM7SideQuests.KerridLetterRequiresFaerieKingExchange` | Complete Kerrid–Faerie King–pipes route, including the missing-pipes refusal, all-food audience cost, state/item transitions, split XP rewards, gold, and active-character-only award. | Add a durable Save/Load boundary when the regional side-quest checkpoint lane is built. |
| `MM7SideQuests.ParsonsQuillRequiresMarkhamExchange` | Complete Norbert–Markham–quill route, including the missing-quill refusal and state/reward assertions. | Add a durable Save/Load boundary when the regional side-quest checkpoint lane is built. |
| Future regional side/promotion suites | Every table row in this catalog. | One success, one meaningful alternate/refusal, state/reward, and durable-state assertion per row. |

## Implementation gates

1. Preserve the Wine Cellar kill-and-exit contract when changing cellar actor or event behavior.
2. Treat all C rows as the public-source behavior contract. Where a later
   implementation exposes a source/event mismatch, record the mismatch and
   resolve it before changing a test oracle; do not turn a guide route into an
   unconditional engine rule.
3. Build the Dark C1-C7 chain only from the Dark rows above, adding normal save boundaries and explicit alignment/recipient assertions.
4. Implement side-quest tests in regional batches, with one positive, one meaningful refusal/alternate outcome, rewards/state assertions, and a save boundary where a quest changes durable world state.
5. Add each verified row to the test-to-contract map before calling this catalog complete.

See also: [Campaign Status](Campaign-Status.md), [Current Delivery Order](Current-Delivery-Order.md), and [MM7 Playtest Recording](MM7-Playtest-Recording.md).

## Exhaustiveness audit — 2026-11-11

The catalog was cross-checked against the MM7 `QuestBit` register rather than
only against existing tests. Every objective-bearing active/finish bit maps to
one of the main, Light, Dark, side, promotion, or Obelisk rows above. The
remaining quest bits are substeps, alternate outcomes, travel gates, failed
states, or post-completion state (for example, the three statuette placements,
war alternatives, tapestry failure, or individual Obelisk discoveries), and
are explicitly required by their parent contracts. The remaining register
entries are intentionally out of scope under the boundary above.

The public-source pass corrected the Monk promotion location and the Archer
promotion action, added the previously omitted Griffin hunt and Obelisk puzzle,
and recorded player-visible side-quest rewards. This establishes the behavior
baseline for Dark-chain work and later regional side-quest implementation; it
does not convert incomplete runtime tests into covered quests.

An upstream event-decoder correction now treats MM7 script monster identifiers
as zero-based for `IsActorKilled` and topic availability checks. Its focused
original-script tests prove both regional Griffin predicates and the Stone City
Troglodyte Queen boundary; the local reporting contracts continue to own the
player-visible reward assertions.

Scripted award grants now trigger a character reaction only for recipients who
receive the award for the first time. This preserves the one-time promotion and
quest-feedback contract without replaying the reaction for a duplicate award.

Indoor encounter points outside every map sector now produce no actor, matching
vanilla MM7. This prevents unreachable void actors from falsifying clear-all
or kill-gated quest fixtures; existing saved actors and valid sector spawns are
unchanged.
