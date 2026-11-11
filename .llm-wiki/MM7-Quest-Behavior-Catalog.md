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

## Shared main path

| ID | Intended behavior | Current test status | Confidence |
| --- | --- | --- | --- |
| M0 | Give Judge Thomas the six required Emerald Island contest items; Markham awards Harmondale and offers departure. Missing any item refuses completion. | C0 and `MM7ScavengerPath`; item acquisition is separately open. | C |
| M1 | Meet the Butler, clear all Castle Harmondale goblins, leave, and report back; this unlocks the dwarf repair route. | C1 and Castle-path controls, including survivor refusal. | C |
| M2 | Hothfarr gives the elixir route; cure all seven petrified dwarves and report to the king to rebuild Harmondale. | C2 and rebuild controls, including one-missing refusal. | C |
| M3 | War actions may favor Erathia or Avlee through Loren Steel and Riverstride alternatives; time then leads to the arbiter decision. | Only the Light continuation is chained. Real/fake/ignored actions and timer outcome are uncovered. | O |
| M4 | Recruit Fairweather for Light or Sleen for Dark and install the candidate at Judge Grey's house; the choice is permanent and changes alignment relations. | Light C3 plus choice/end smoke tests. Full time-selector and Dark route absent. | C for outcome; O for exact timing conflict. |

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
| D4 | Dark Shade requires Clanker’s Laboratory magical defenses/controls disabled. Sources disagree on whether a full clear is required. | Event-script/original observation before a kill-all assertion. | O |
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
| Harmondale raid/Malwick | Preserve existing raid/mercenary controls, then reconcile giver, timing, rewards, and town consequence against two sources before expansion. | Partial focused coverage. | O |
| War and Niles routes | Treat Loren/false-Loren, real/false plans, Niles/tapestry, deadlines, and Harmondale attack as branch/time-sensitive contracts, not ordinary fetch quests. | Uncovered matrix. | O |

## Promotions (side-quest contracts)

| Group | Intended behavior | Test status | Confidence |
| --- | --- | --- | --- |
| First promotions | Monk meditation pool; Ranger Faerie King; Druid three monoliths; Archer Red Dwarf lift; Knight Haunted Mansion; Paladin Wromthrax; Cleric pirate map; Thief Markham vase; Sorcerer six-part golem. Matching class receives promotion; others receive honorary promotion. | Uncovered as a suite. | C |
| Light second promotions | Master (Baa priest), Ranger Lord (Heart of Wood), Arch Druid (Zokarr remains), Master Archer (Perfect Bow), Champion (five Knight arena wins), Hero (rescue Alice), Priest of Light (purify Sun altar), Spy (Watchtower counterweight), Archmage (Divine Intervention). Requires Light alignment and first promotion. | Uncovered. | C |
| Dark second promotions | Ninja (Ashwar tomb), Bounty Hunter (10,000 bounty gold), Warlock (dragon egg), Sniper (Perfect Bow), Black Knight (Navan treasury), Villain (capture Alice), Priest of Dark (deface Sun altar), Assassin (Lady Carmine), Lich (Proving Grounds jars). Requires Dark alignment and first promotion. | Uncovered. | C |

## Implementation gates

1. Correct the Wine Cellar test before using its completion state as a Light-chain oracle.
2. Resolve every `O` row with an original-game observation or narrow source-event audit; do not turn a guide route into an unconditional engine rule.
3. Build the Dark C1-C7 chain only from the Dark rows above, adding normal save boundaries and explicit alignment/recipient assertions.
4. Implement side-quest tests in regional batches, with one positive, one meaningful refusal/alternate outcome, rewards/state assertions, and a save boundary where a quest changes durable world state.
5. Add each verified row to the test-to-contract map before calling this catalog complete.

See also: [Campaign Status](Campaign-Status.md), [Current Delivery Order](Current-Delivery-Order.md), and [MM7 Playtest Recording](MM7-Playtest-Recording.md).
