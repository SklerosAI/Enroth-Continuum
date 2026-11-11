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

Primary sources: [RPGClassics side and promotion guide](https://shrines.rpgclassics.com/pc/mm7/quests.shtml),
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
| L3 | Crag Hack's Wine Cellar task requires the Elder Vampire to be defeated before return. | **Conflict:** C5 currently obtains completion by leaving; this is not accepted as intended coverage. | C |
| L4 | Caneghem requires the Case of Soul Jars from Castle Gloaming. | C5 and item/adviser tests. | C |
| L5 | Once the three advisers are complete, Robert requests Tolberti's control cube; kill Tolberti, loot the cube, return it, and gain Blaster training. | C5 and Cube combat/hand-in tests; add reward assertion. | C |
| L6 | Resurecta sends the party after Xenofex in Colony Zod; the normal route includes Roland/key/control access, Xenofex's death, then Resurecta hand-in. | C6 proves combat and hand-in, but teleports past Roland/access route. | C; route gap |
| L7 | Acquire/equip four wetsuits, reach Shoals and Lincoln, restore power, take the Oscillation Overthruster, return to Resurecta, and receive the Light ending. | C7; individual gate controls and ending are present. | C |

## Dark main path

| ID | Intended behavior | Required future contract | Confidence |
| --- | --- | --- | --- |
| D1 | Meet Archibald in Castle Gloaming, complete the Breeding Zone proof, exit, and unlock Dark-side adviser progression/Dark Magic membership. Killing every inhabitant is not the stated completion rule. | First Dark chain chapter. | C |
| D2 | Kastore requires both altar pieces, using the Dark counterpart route and returning them to Kastore. | Reuse mechanics but assert recipient and Dark alignment behavior. | C |
| D3 | Maximus requires the Soul Jars from Thunderfist Mountain, not the Light-side Gloaming route. | Dedicated acquisition and hand-in test. | C |
| D4 | Dark Shade requires Clanker's Laboratory's magical/teleport defenses disabled; it is not a kill-all dungeon objective. | Dedicated map-event and return test. | C |
| D5 | After the three advisers, Tolberti sends the party to assassinate Robert in Celeste; loot the cube/Blaster and return it to Tolberti. | Dedicated mirrored-but-distinct quest test. | C |
| D6 | Complete the Xenofex/Colony Zod progression and return to Kastore. | Dark C6, including access route and recipient. | C |
| D7 | Complete Lincoln/Overthruster progression and return it to Kastore for the Dark ending. | Dark C7, including ending and post-credits behavior. | C |

## Non-branch side quests

| Quest | Intended completion/outcome | Test status | Confidence |
| --- | --- | --- | --- |
| Missing Contestants | Return the Contestant Shield from Emerald Island’s Dragon Cave to Markham. | Uncovered. | C |
| Lantern of Light | Return the Lantern from Barrow II to Tarin Withern. | Uncovered. | C |
| Davrik’s Signet Ring | Recover the Bandit Caves ring and return it to Davrik. | Uncovered. | C |
| Darron’s brother | Find the White Cliff evidence/deck and return to Darron Temper; unlocks the Arcomage deck. | Uncovered. | C |
| Three statuettes | Place Eagle/Knight/Angel statuettes at their three shrines, then return to Thom Lumbra. | Uncovered. | C |
| Kerrid letter | Deliver Kerrid’s letter to the Faerie King, receive pipes, return them; the audience consumes party food. | Uncovered. | C |
| Norbert’s quill | Deliver Norbert’s letter to Markham, return Parson’s Quill. | Uncovered. | C |
| Arcomage champion | Win once in each of thirteen taverns, then return to Gina Barnes. | Uncovered. | C |
| Three paintings | Return the Angel, Archibald, and Roland paintings to Visconti. | Uncovered. | C |
| Haldar’s remains | Recover remains from the Maze and return to Mazim Dusk. | Uncovered. | C |
| Season’s Stole | Recover the stole from the Hall of the Pit and return it to Gary Zimm. | Uncovered. | C |
| Troglodytes | Clear Stone City’s lower-tunnel troglodytes and report to Spark Burnkindle. | Uncovered. | C |
| Malwick's favor/Niles debt | Taking Malwick's Wand of Fireballs on Emerald Island conditionally starts the later Niles chain. Within six weeks of choosing a path, the party receives the Guild message on entering Harmondale; it then has two weeks to visit Niles and two more weeks to return the path-appropriate allied-castle tapestry. | Partial focused coverage; expand into the explicit clock/trigger cases. | C |
| War alternatives | Cover Loren real/decoy/decline, Riverstride real/decoy/decline, the resulting war score, trumpet recipient, and the neutral option independently. | Uncovered matrix. | C |
| Niles/tapestry | Recover the allied-castle tapestry (Lambent on Light, Gloaming on Dark) and return it to Niles. Taking it makes that allied region hostile for one month. Missing either two-week deadline causes Goblins and Swordsmen to attack Harmondale; shops/services close until the invaders are cleared. | Uncovered. | C |

## Promotions (individual side-quest contracts)

Every promotion test must assert the intended class receives the class change,
an ineligible party member receives only the honorary outcome, and a second-tier
test rejects the wrong alignment or a missing first promotion. All rows are
uncovered unless stated otherwise.

| Tier | Quest and intended completion | Confidence |
| --- | --- | --- |
| First | Monk → Initiate: use the meditation pool in the Hall under the Hill. | C |
| First | Ranger → Hunter: complete the Faerie King's task. | C |
| First | Druid → Great Druid: activate the three druid monoliths. | C |
| First | Archer → Warrior Mage/Battle Mage: recover the Red Dwarf lift belt. | C |
| First | Knight → Cavalier: clear the Haunted Mansion task. | C |
| First | Paladin → Crusader: defeat Wromthrax. | C |
| First | Cleric → Priest: recover the pirate map; its other intended effect is opening the Evenmorn route. | C |
| First | Thief → Rogue: return Markham's vase. | C |
| First | Sorcerer → Wizard: recover and assemble all six golem components; separately cover the good/bad head choice. | C |
| Light second | Monk → Master: help the priest of Baa. | C |
| Light second | Ranger → Ranger Lord: return the Heart of Wood. | C |
| Light second | Druid → Arch Druid: return Zokarr's remains. | C |
| Light second | Archer → Master Archer: return the Perfect Bow. | C |
| Light second | Knight → Champion: win five Knight Arena bouts. | C |
| Light second | Paladin → Hero: rescue Alice. | C |
| Light second | Cleric → Priest of Light: purify the Sun altar. | C |
| Light second | Thief → Spy: recover the Watchtower counterweight. | C |
| Light second | Sorcerer → Archmage: invoke Divine Intervention. | C |
| Dark second | Monk → Ninja: complete Ashwar's tomb task. | C |
| Dark second | Ranger → Bounty Hunter: turn in 10,000 gold in bounties. | C |
| Dark second | Druid → Warlock: return the dragon egg. | C |
| Dark second | Archer → Sniper: return the Perfect Bow. | C |
| Dark second | Knight → Black Knight: complete the Navan treasury task. | C |
| Dark second | Paladin → Villain: capture Alice. | C |
| Dark second | Cleric → Priest of Dark: deface the Sun altar. | C |
| Dark second | Thief → Assassin: complete Lady Carmine's dagger task. | C |
| Dark second | Sorcerer → Lich: return the Proving Grounds soul jars. | C |

## Current-test-to-contract map

| Existing test family | Contract it is allowed to claim | Required correction or follow-up |
| --- | --- | --- |
| `MM7CampaignChainC0`, `MM7ScavengerPath` | M0 delivery, refusal when one contest item is missing, Judge and ship progression. | Add separate acquisition checks for all six items. |
| `MM7CampaignChainC1`, `MM7CastlePath` | M1 Butler/goblin/report sequence and survivor refusal. | None before broader side coverage. |
| `MM7CampaignChainC2`, `MM7CastleRebuild` | M2 elixir, seven-dwarf, rebuild sequence and one-missing refusal. | Keep its save boundary. |
| `MM7CampaignChainC3`, `MM7Campaign` | Light Arbiter selection and post-choice house relation. | Add M3–M5 timer, war-score, trumpet/neutral, and Dark selection cases. |
| `MM7CampaignChainC4`, `MM7LightWalls`, `MM7LightItems` | L1 key/pillar path and no-kill player rule. | Audit the unreachable local protected-group predicate. |
| `MM7CampaignChainC5`, `MM7LightAdvisors`, `MM7PitCubeAcquisition` | L2, L4, L5 item/return progression. | Replace the L3 exit-bit oracle with an actual Elder Vampire defeat; assert Blaster training reward. |
| `MM7CampaignChainC6`, `MM7LightXenofex`, `MM7XenofexCombat` | L6 Xenofex combat and Light hand-in. | Add Roland/key/control access route. |
| `MM7CampaignChainC7`, `MM7OverthrusterRoute` | L7 wetsuits, Lincoln, Overthruster, Light ending. | Retain all individual gate controls. |
| Future `MM7DarkCampaignChainC1`–`C7` | D1–D7 through normal, durable save boundaries. | Include recipient, alignment, combat/event and ending assertions named above. |
| Future regional side/promotion suites | Every table row in this catalog. | One success, one meaningful alternate/refusal, state/reward, and durable-state assertion per row. |

## Implementation gates

1. Correct the Wine Cellar test before using its completion state as a Light-chain oracle.
2. Treat all C rows as the public-source behavior contract. Where a later
   implementation exposes a source/event mismatch, record the mismatch and
   resolve it before changing a test oracle; do not turn a guide route into an
   unconditional engine rule.
3. Build the Dark C1-C7 chain only from the Dark rows above, adding normal save boundaries and explicit alignment/recipient assertions.
4. Implement side-quest tests in regional batches, with one positive, one meaningful refusal/alternate outcome, rewards/state assertions, and a save boundary where a quest changes durable world state.
5. Add each verified row to the test-to-contract map before calling this catalog complete.

See also: [Campaign Status](Campaign-Status.md), [Current Delivery Order](Current-Delivery-Order.md), and [MM7 Playtest Recording](MM7-Playtest-Recording.md).
