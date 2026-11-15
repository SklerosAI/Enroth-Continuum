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
| M3 | Catherine's Loren Steel rescue and Parson's Riverstride-plans request each have a 28-day response window. The party may complete the real task, deliver its decoy, or ignore it until timeout; those choices determine the war result unless the trumpet is used. | `MM7WarAlternatives` covers both real, decoy and timeout matrices through the authored dialogue and physical Loren/plans sources. It also fixes and guards the two timeout blocks that previously fell through into real-task rewards. The reveal/treason failure branches remain open. | C |
| M4 | From day 56 until day 84 after the war clock starts, entering Tularean Forest activates the Gryphonheart's Trumpet battle. Give it to Erathia or Avlee to decide the war, or to Judge Grey after a balanced record to preserve Harmondale independence. Judge Grey dies on day 112, then enables the Arbiter selection. | `MM7WarAlternatives` drives the authored Harmondale timer and Tularean reload window, takes the physical trumpet from chest 0, and covers Catherine, Parson and Judge Grey hand-ins. The balanced real/real neutral route continues through the 112-day messenger and a durable Dark checkpoint. The two unplaced envoy event handlers and remaining bit-result combinations are open. | C |
| M5 | Recruit Fairweather for Light or Sleen for Dark and install the candidate at Judge Grey's house. The choice is permanent and changes alignment relations. | Light C3 and Dark C3 now cover both selectors in independent continuous profiles. Dark C3 continues the common C0-C2 saves, waits through the authored 112-day messenger, recruits Sleen at the Deyja tavern, installs him at the Arbiter house, verifies Evil alignment and Archibald activation, then writes a structurally checked chapter save. `MM7WarAlternatives.BalancedWarAndNeutralTrumpetCanInstallDarkArbiter` separately reaches the same result through the balanced-war route and preserves it through normal Save/Load. | C |

## Light main path

| ID | Intended behavior | Current test status | Confidence |
| --- | --- | --- | --- |
| L1 | Gavin Magnus sends the party through Walls of Mist; avoid killing its protected inhabitants, solve the key/pillar route, exit, and report back to join the Light guild. | C4 and focused Walls tests. Local group-ID predicate is unreachable, so retain the no-kill player contract and audit the predicate separately. | C / O for local predicate |
| L2 | Resurecta requires both altar pieces from the Light and Dark temples; return both after their dungeon interactions. | C5 plus item/adviser tests. | C |
| L3 | Crag Hack's Wine Cellar task requires the Elder Vampire to be defeated before return. | The shipped quest actor is now activated only for the exact live Wine Cellar quest transaction. `MM7CombatHarness.WineCellarAttackInputDamagesAndKillsTarget` kills it through attack input, while completed/killed blockers keep it disabled and `MM7LightItems.WineCellarExitDoesNotReplaceVampireKill` retains the false-completion guard and adviser hand-in. | C |
| L4 | Caneghem requires the Case of Soul Jars from Castle Gloaming. | C5 and item/adviser tests. | C |
| L5 | Once the three advisers are complete, Robert requests Tolberti's control cube; kill Tolberti, loot the cube, return it, and gain Blaster training. | C5 and the physical Pit cube combat/loot route remain; `MM7LightAssassination.RefusalFullRewardAndDuplicateProtection` verifies cube refusal/consumption, quest/history state, 250,000 XP per character, all-character award, novice Blaster only for untrained characters, preserved higher mastery, reputation, throne relocation, topic cleanup and no repeat reward. | C |
| L6 | Resurecta sends the party after Xenofex in Colony Zod; the normal route includes Roland/key/control access, Xenofex's death, then Resurecta hand-in. | Light C6 accepts Resurecta's assignment, follows Mount Nighon through Thunderfist Mountain and the Tunnels to Eofol, receives Archibald's Land-of-the-Giants Blaster, enters Colony Zod, rescues Roland, consumes his one-use key, activates both route controls, kills Xenofex by player attack, leaves through the authored exit, uses the return altar, waits through Lambent's 5am closure and completes Resurecta's hand-in before `save006.mm7`. The existing C7 ending passes from that strengthened save. | C |
| L7 | Acquire/equip four wetsuits, reach Shoals and Lincoln, restore power, take the Oscillation Overthruster, return to Resurecta, and receive the Light ending. | C7; individual gate controls and ending are present. | C |

## Dark main path

| ID | Intended behavior | Required future contract | Confidence |
| --- | --- | --- | --- |
| D1 | Meet Archibald in Castle Gloaming, complete the Breeding Zone proof, exit, and unlock Dark-side adviser progression/Dark Magic membership. Killing every inhabitant is not the stated completion rule. | Dark C4 continues the normal C3 save through Archibald's night throne-room assignment, the one authored proof face, one of the two authored Pit exits, the Gloaming hand-in and all three newly assigned, distinct Pit adviser houses before a normal save. The independent `MM7DarkGuild` assignment, marker-only refusal and hand-in tests remain focused controls. | C |
| D2 | Kastore requires both altar pieces, using the Dark counterpart route and returning them to Kastore. | Dark C5 accepts Kastore's task, opens the Light Temple chest, opens the Dark Temple altar lid and chest, and returns both physical pieces for consumption through Kastore's runtime-selected Pit house. `MM7DarkGuild.KastoreRequiresBothAltarPieces` retains the one-piece refusal control. | C |
| D3 | Maximus requires the Soul Jars from Thunderfist Mountain, not the Light-side Gloaming route. | Dark C5 accepts Maximus's task, discovers the authored Thunderfist chest event from its contents, opens the physical chest face, takes the case and returns it for consumption. The focused assignment/refusal and chest-source controls remain. | C |
| D4 | Dark Shade requires Clanker's Laboratory's magical/teleport defenses disabled; it is not a kill-all dungeon objective. | Dark C5 accepts Dark Shade's task, activates the authored defense-disable face and completes the hand-in. The focused map-state source and pre-marker refusal controls remain. | C |
| D5 | After the three Dark advisers, Tolberti in the Pit sends the party after Robert the Wise in Celeste. Obtain Robert's control cube by killing or stealing it, then return it to Tolberti. Robert drops a Blaster as well as the cube; the hand-in grants every party member novice Blaster skill. Normal Blaster training is thereby enabled, with Dark expert/master/grandmaster trainers in the Pit/Gloaming. | Dark C5 takes Tolberti's assignment, enters Robert's actual Celeste Small House, kills the live cube-and-Blaster carrier by player attack, targets and loots the fallen actor, returns the cube, verifies its consumption and novice Blaster skill for all four characters, and proves Kastore's Gloaming relocation. The relocation occurs with Robert's completion rather than immediately after the adviser trio. | C |
| D6 | Kastore's Dark quest requires the Hive/Colony Zod route: rescue Roland, activate the two route controls that open the way, kill Xenofex, then return to Kastore. | Dark C6 continues `save005.mm7`, accepts Kastore's assignment, follows Mount Nighon through Thunderfist Mountain and the Tunnels to Eofol, receives Archibald's Land-of-the-Giants dialogue, enters Colony Zod, rescues Roland, consumes his key, activates both route controls, kills Xenofex by player attack, leaves through the authored exit, uses the return altar and completes Kastore's hand-in before `save006.mm7`. Focused assignment/refusal and Roland/control tests remain independent guards. | C |
| D7 | After the Xenofex return, use four wetsuits (only Blasters may remain equipped) to reach Lincoln from Avlee, re-equip inside, restore Lincoln power, retrieve the Oscillation Overthruster, and return it to Kastore for the Dark ending. | Dark C7 accepts Kastore's final topic, discovers and opens both authored Gloaming faces whose chests contain wetsuits, equips all four suits through the inventory, walks across Avlee's underwater border, takes the authored Shoals-to-Lincoln transition, powers the Lincoln console, physically retrieves the Overthruster, exits, returns to Kastore and completes the Dark ending before `save007.mm7`. Focused assignment, route and ending tests remain independent guards. | C |

## Non-branch side quests

| Quest | Intended completion/outcome | Test status | Confidence |
| --- | --- | --- | --- |
| Missing Contestants | Return the Contestant Shield from Emerald Island’s Dragon Cave to Markham; award is 1,000 XP and 1,000 gold. | `MM7SideQuests.MissingContestantsRequiresDragonCaveShield` dynamically finds Markham and his offer, proves the no-shield refusal, clicks the physical shield on the floor of Emerald Island's Dragon's Lair, and completes the real return. It verifies quest clearance, item consumption, 1,000 gold, 1,000 XP per character and the award for all four characters. | C |
| Lantern of Light | Tarin Withern assigns the Lantern recovery; return the physical Lantern from the Barrow II chest to receive 5,000 XP per character, 1,000 gold, and the Harmondale award. | `MM7SideQuests.LanternOfLightRequiresBarrowChest` dynamically finds Tarin, proves acceptance and no-item refusal, loots the event-mapped Barrow II chest through the chest UI, and verifies normal hand-in consumption and rewards. | C |
| Davrik’s Signet Ring | Recover the Bandit Caves ring after accepting Davrik's task, then return it for 5,000 XP per character, 5,000 gold, and the Harmondale award. | `MM7SideQuests.DavriksSignetRingRequiresBanditCavesChest` dynamically finds Davrik and the tavern, proves the empty-handed refusal, loots the quest-gated Bandit Caves chest through its normal UI, and verifies the ring consumption, state transition and rewards. | C |
| Darron’s brother | Find the White Cliff evidence/deck and return to Darron Temper; retain the Arcomage deck, receive 2,000 XP and 750 gold. | `MM7SideQuests.DarronsDeckIsPhysicallyAcquiredAndReturned` reaches Darron's real Harmondale house dialogue, accepts the task, finds the persistent White Cliff Cave deck object, clicks it through the world-item path, then returns it to Darron. It verifies active-state clearance, retained deck, 750 gold and 2,000 XP per character. | C |
| Three statuettes | Place Eagle/Knight/Angel statuettes at their three shrines, then return to Thom Lumbra for 50,000 XP and 50,000 gold. | `MM7SideQuests.ThreeStatuettesRequireChestsAndShrines` reaches Thom Lumbra's real offer, proves the incomplete return refusal, takes the Knight/Eagle/Angel statuettes through the Grand Temple of the Sun, Grand Temple of the Moon and Maze chest UIs, places them through the authored Avlee/Tatalia/Bracada outdoor shrine faces, and verifies item consumption, all three placement bits, quest clearance, 50,000 gold, 50,000 XP per character and the award for all four characters. The exact production-Save writer and separate normal-Load reader preserve that state with unchanged slot bytes. | C |
| Kerrid letter | Deliver Kerrid’s letter to the Faerie King, receive pipes, and return them. The audience consumes all party food and grants 2,000 XP per character; Kerrid grants another 5,000 XP per character and 1,000 gold. The script writes the journal award only to the active character. | `MM7SideQuests.KerridLetterRequiresFaerieKingExchange` dynamically finds Kerrid, the Faerie King, and the Hall Under the Hill audience event. It covers the real offer and physical letter, the missing-pipes refusal, all-food consumption, letter-to-pipes exchange, quest-bit transitions, 7,000 total XP per character, 1,000 gold, pipe consumption, and the active-character-only award behavior. | C |
| Norbert’s quill | Deliver Norbert’s letter to Markham, return Parson’s Quill for 5,000 XP and 2,000 gold. | `MM7SideQuests.ParsonsQuillRequiresMarkhamExchange` completes the Emerald contest and ship departure through input, accepts Thrush's letter, proves his visible return topic refuses the missing quill, exchanges the physical letter with relocated Markham in his Tatalia manor, and returns the quill. It verifies item/quest transitions, 2,000 gold, -5 reputation, the Erathia award and 5,000 XP per character. | C |
| Arcomage champion | Win once in each of thirteen taverns, then return to Gina Barnes for 50,000 XP, 100,000 gold, and the chest reward. | `MM7SideQuests.ArcomageChampionRequiresAllTavernResultsAndGina` reaches Gina's real offer, proves the incomplete return refusal, records one production result for each of the thirteen authored Arcomage tavern IDs, proves a repeat win does not repeat its stake payout, and completes Gina's hand-in. It verifies all win flags, the champion bit, 100,000-gold and 50,000-XP-per-character rewards, the award for all four characters, and takes the Forge Gauntlets through the authored Erathia reward chest UI. The exact production-Save writer and separate normal-Load reader preserve that state with unchanged slot bytes. `Issues.Issue388` separately enters the live card loop and plays cards; this deterministic quest test does not claim thirteen complete card matches. | C |
| Three paintings | Return the Angel, Archibald, and Roland paintings to Visconti for 50,000 XP and 50,000 gold. | `MM7SideQuests.ThreePaintingsRequireAuthoredWallPickups` reaches Visconti's real offer, proves the empty-handed refusal, takes the Angel painting from the Haunted Mansion and the Archibald/Roland paintings from Castle Gryphonheart through their authored clickable wall events, then verifies all three taken bits, item consumption, quest clearance, 50,000 gold and 50,000 XP per character. The exact production-Save writer and separate normal-Load reader preserve that state with unchanged slot bytes. | C |
| Haldar’s remains | Mazim Dusk assigns recovery of Haldar's physical remains from the Maze; return them for 50,000 XP per character, 35,000 gold, and the Nighon award. | `MM7SideQuests.HaldarsRemainsRequireMazeChest` dynamically finds Mazim, proves acceptance and no-item refusal, loots the event-mapped Maze chest through the chest UI, and verifies normal hand-in consumption and rewards. | C |
| Season’s Stole | On the Light path, accept Gary Zimm's task, recover the Stole from the Hall of the Pit, and return it for 7,500 XP per character and 7,500 gold. The script writes the journal award only to the active character. | `MM7SideQuests.SeasonsStoleRequiresHallOfThePitChest` reaches Gary's real Bracada house and Light-gated offer, proves the missing-Stole refusal, takes the physical Stole through the Hall of the Pit chest UI, and completes the return. It verifies item and quest-state transitions, 7,500 gold, 7,500 XP per character, and the active-character-only award behavior. | C |
| Troglodytes | Clear Stone City’s lower-tunnel troglodytes and report to Spark Burnkindle for 5,000 XP per character, 2,500 gold, -10 reputation, and the lower-city award. The Nighon passage is excluded. | `MM7SideQuests.TroglodytesRequireLowerCityClearance` proves assignment, refusal with one survivor, and the complete-clear state/reward contract. Direct combat remains separate. | C |
| Griffins | On the Dark path, kill every Griffin in both Erathia and Bracada Desert, then report to Seth Drakkson for 10,000 XP and 5,000 gold. | `Issues.Issue2868a` proves the per-map all-Griffin predicate (including the former Gold Golem mismatch); `Issue673d` covers assignment/report completion and reputation. Add XP, gold, award and refusal assertions. | C |
| Malwick's favor/Niles debt | Taking Malwick's Wand of Fireballs on Emerald Island conditionally starts the later Niles chain. The Light/Dark path choice starts counter 5; after 1,008 hours, entering Harmondale delivers `ITEM_MESSAGE_FROM_MR_STANTLEY` (item 704) and starts the two-week Niles-visit deadline. | `MM7MalwickNiles.RefusalKeepsMalwickProposalAvailable` uses Malwick's placed Emerald Island street NPC and covers proposal refusal, the repeat offer and acceptance with the Wand. `SixWeekTriggerAndMissingTapestryRefusal` proves the 1,007/1,008-hour boundary through Harmondale's authored reload event, message delivery, Niles's real Tatalia entrance and the missing-tapestry refusal. The path-choice-to-counter seam is covered separately by Campaign Chain C3, not replayed continuously here. | C |
| War alternatives | Cover Loren real/decoy/ignored timeout, Riverstride real/decoy/ignored timeout, the bit-derived result matrix, trumpet recipient, and the neutral option independently. | `MM7WarAlternatives` covers both three-way assignment matrices, the three production-accessible trumpet recipients, balanced real/real neutral resolution and Dark Arbiter installation. The durable writer/reader preserves the neutral/Dark state with unchanged slot bytes. Reveal/treason failures, unplaced envoy reachability, balanced decoy/decoy and the remaining result-bit combinations are open. | C |
| Niles/tapestry | Recover the allied-castle tapestry (Lambent on Light, Gloaming on Dark) and return it to Niles. Taking it makes that allied region hostile for one month. The visit deadline is 336 hours; accepting Niles's task resets the same counter and starts a 672-hour return deadline. Missing either deadline causes Goblins and Swordsmen to attack Harmondale; shops/services close until the invaders are cleared. | `MM7MalwickNiles.PhysicalTapestriesCompleteBothPathReturns` selects Niles's distinct Light/Dark offers, clicks the authored Castle Lambent/Gloaming tapestry faces, returns each item through Niles's real Tatalia dialogue, and verifies consumption, completion, +5 reputation, and no XP, gold, bank-gold or award change. `AlliedCastleHostilityExpiresAtThirtyDays` proves both allied castles remain aggressive at 719 hours and clear at exactly 720. `MissedDeadlinesStartRaidAndCloseHarmondaleServices` proves both exact deadline failures, bank loss, live group-9 invaders and a closed service; `MM7Campaign.HarmondaleRaid` proves both attacker groups must be cleared before services reopen. Separate admitted writer/reader pairs preserve the successful return and failed invasion in fresh processes with unchanged slot bytes. | C |
| Obelisk puzzle | Examine all fourteen regional Obelisks, then visit the Evenmorn stone circle between midnight and 1 AM to receive the 100,000-gold treasure chest and its items. | `MM7SideQuests.ObeliskPuzzleRequiresAllRegionsAndMidnight` discovers and executes each authored local Obelisk binding, proves the treasure remains hidden with one discovery missing and at 1 AM, then exposes, opens and empties the real Evenmorn chest at midnight. It verifies all discovery and treasure bits, the scripted 100,000 gold, chest gold and physical chest items. The exact production-Save writer and separate normal-Load reader preserve every bit, the reward and emptied chest with unchanged slot bytes. | C |

## Promotions (individual side-quest contracts)

Every promotion test must assert the intended class receives the class change,
an ineligible party member receives only the honorary outcome, and a second-tier
test rejects the wrong alignment or a missing first promotion. First promotions
award 30,000 XP to a qualifying class and 15,000 XP honorary; second promotions
award 80,000 XP qualifying and 40,000 XP honorary. Rows name their accepted
test when behavior coverage exists.

| Tier | Quest and intended completion | Confidence |
| --- | --- | --- |
| First | Monk → Initiate: accept from Bartholomew Hume, find and use the meditation pool in Zokarr's Tomb in the Dwarven Barrows. `MM7BarrowQuests.MeditationPoolPromotesMonksAndHonorsCompanionsOnce` uses the real pool event and proves missing-prerequisite refusal, qualifying/honorary awards and XP, topic cleanup, no gold and duplicate prevention. | C |
| First | Ranger → Hunter: complete the Faerie King's task. `MM7Promotions.RangerHunterRealDialoguePacket` uses Sower's house, the Hall Under the Hill audience event and the real dialogue transaction; it covers the wrong prerequisite, qualifying/honorary outcomes, exact XP/topics/qbits and duplicate prevention. | C |
| First | Druid → Great Druid: activate the three druid monoliths. `MM7BarrowGreatDruid.PhysicalStonehengesCompletePromotionOnce` takes Anthony Green's real offer, proves zero- and one-Stonehenge refusals, physically clicks the authored event-401 faces in Evenmorn, Tatalia and Avlee, and verifies qualifying/honorary class, award and XP outcomes, no gold, reputation, bits, topic cleanup and duplicate prevention. | C |
| First | Archer → Warrior Mage: use the supplied Worn Belt to sabotage the Red Dwarf Mines lift, then return. `MM7Promotions.ArcherWarriorMagePhysicalPromotionPacket` uses Steagal Snick's real house dialogue, proves premature refusal, clicks the authored mine-machine face, consumes the belt, and verifies qualifying/honorary XP and awards, gold, reputation, topics, bits and duplicate prevention. | C |
| First | Knight → Cavalier: clear the Haunted Mansion task. `MM7Promotions.KnightCavalierPhysicalPromotionPacket` uses Frederick Org's real house dialogue, proves premature and live-actor exit refusals, requires the authored Mansion map-leave clear predicate, and verifies qualifying/honorary XP and awards, no-gold behavior, reputation, topics, bits and duplicate prevention. | C |
| First | Paladin → Crusader: defeat Wromthrax. `MM7Promotions.PaladinCrusaderAttackInputPromotionPacket` takes Quixote's real offer, kills Wromthrax through bounded attack input, advances the authored 210-second objective timer, and completes Quixote's real hired-NPC dialogue. It verifies qualifying/honorary awards and XP, no gold, reputation, bits, topic/greeting changes, dismissal after dialogue closes and duplicate prevention. | C |
| First | Cleric → Priest: recover the pirate map; its other intended effect is opening the Evenmorn route. `MM7PriestPromotion.ClericPriestPhysicalPromotionPacket` takes Falk's real offer, proves the no-map refusal, physically opens the Tidewater chest and returns the held map. It verifies qualifying/honorary outcomes, map consumption, bits, topic replacement, 5,000 gold, reputation, duplicate prevention and the Bracada boat's false-before/true-after Evenmorn availability. | C |
| First | Thief → Rogue: accept William Lasker’s request in the Erathian Sewers, take Markham’s Vase from the Markham Manor fireplace, and return it. `MM7Promotions.MarkhamVaseRequiresPhysicalPickup` proves offer, no-vase refusal, real map pickup, hand-in consumption, promotion/honorary split, XP and gold. | C |
| First | Sorcerer → Wizard: recover and assemble all six golem components; separately cover the good/bad head choice. `MM7WizardPromotion` takes Thomas Grey's real offer, physically loots every authored outdoor component source, installs the parts through the Golem's dialogue, performs the required extra assembly confirmation and completes the exact qualifying/honorary transaction. Separate good and dented head routes prove friendly versus hostile Castle actors, NPC dismissal, duplicate prevention and no gold or reputation reward. | C |
| Light second | Initiate → Master: on the Light path, accept from Bartholomew Hume, kill the High Priest of Baa in Avlee's Temple of Baa, collect his cloak, then return to Hume. | C |
| Light second | Ranger → Ranger Lord: return the Heart of Wood. | C |
| Light second | Druid → Arch Druid: retrieve Zokarr IV's skull, return it to his tomb, then report back. `MM7BarrowDialogue.ArchDruidUsesNighonSkullSourceAndZokarrReturn` covers Green's real Light-only offer and premature-return refusal, the live Nighon Tunnels event-196 skull source, Zokarr's physical deposit face, Green's exact qualifying/honorary reward transaction and duplicate prevention. The apparent Hothfarr/event-178 handoff is unreachable dead original content, not a missing OpenEnroth topic. | C |
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
| `MM7CampaignChainC5`, `MM7LightItems`, `MM7LightAdvisors`, `MM7PitCubeAcquisition`, `MM7CombatHarness` | L2, L3 live Elder Vampire input combat plus exit/hand-in, L4, and L5 physical item/return/full reward progression. | Preserve the narrow Wine Cellar activation conditions and Tolberti duplicate protection. |
| `MM7CampaignChainC6`, `MM7LightXenofex`, `MM7XenofexCombat` | L6 authored Nighon/Thunderfist/Eofol/Land/Zod access, Archibald's Blaster, Roland/key/controls, Xenofex combat, return altar and Light hand-in. | Preserve the full route while reducing approach teleports during ordinary-movement strengthening. |
| `MM7CampaignChainC7`, `MM7OverthrusterRoute` | L7 wetsuits, Lincoln, Overthruster, Light ending. | Retain all individual gate controls. |
| `MM7DarkCampaignChainC3-C7` | The separately marked Dark profile reuses common C0-C2; C3 completes the Arbiter route, C4 completes Archibald/Breeding Zone, C5 completes all three physical adviser objectives plus Robert's live combat/cube hand-in, C6 completes the authored Colony Zod access and Xenofex return, and C7 completes the physical Lincoln/Overthruster route and Dark ending through normal chapter saves. | Preserve the chain while strengthening ordinary movement/combat and later graphical acceptance. |
| `MM7SideQuests.MissingContestantsRequiresDragonCaveShield`, `MM7SideQuestCheckpoint` | Complete Markham route, including offer, missing-shield refusal, physical Dragon's Lair floor pickup, state/item transitions, XP, gold, all-character award and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.KerridLetterRequiresFaerieKingExchange`, `MM7SideQuestCheckpoint` | Complete Kerrid–Faerie King–pipes route, including the missing-pipes refusal, all-food audience cost, state/item transitions, split XP rewards, gold, active-character-only award and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.ParsonsQuillRequiresMarkhamExchange`, `MM7SideQuestCheckpoint` | Complete Norbert–Markham–quill route, including the missing-quill refusal, state/reward assertions and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.SeasonsStoleRequiresHallOfThePitChest`, `MM7SideQuestCheckpoint` | Complete Light-path Gary Zimm route, including the missing-item refusal, physical Hall chest acquisition, state/item transitions, XP, gold, active-character-only award and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.ThreePaintingsRequireAuthoredWallPickups`, `MM7SideQuestCheckpoint` | Complete Visconti route, including offer, empty-handed refusal, all three authored wall pickups, taken bits, item/quest transitions, XP, gold and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.ThreeStatuettesRequireChestsAndShrines`, `MM7SideQuestCheckpoint` | Complete Thom Lumbra route, including offer, incomplete-state refusal, all three authored dungeon chests and outdoor shrine faces, placement bits, item/quest transitions, award, XP, gold and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| `MM7SideQuests.ObeliskPuzzleRequiresAllRegionsAndMidnight`, `MM7SideQuestCheckpoint` | Complete all fourteen authored regional discovery bindings, the one-missing and wrong-hour refusals, the midnight Evenmorn treasure event, chest looting, rewards and fresh-process Save/Load persistence. | The independent route boundary is complete; continuous regional progression remains separate. |
| Future regional side/promotion suites | Every table row in this catalog. | One success, one meaningful alternate/refusal, state/reward, and durable-state assertion per row. |

## Implementation gates

1. Preserve the Wine Cellar kill-and-exit contract when changing cellar actor or event behavior.
2. Treat all C rows as the public-source behavior contract. Where a later
   implementation exposes a source/event mismatch, record the mismatch and
   resolve it before changing a test oracle; do not turn a guide route into an
   unconditional engine rule.
3. Preserve both complete C0-C7 campaign profiles while strengthening their named fixture, movement, combat and graphical gaps.
4. Implement side-quest tests in regional batches, with one positive, one meaningful refusal/alternate outcome, rewards/state assertions, and a save boundary where a quest changes durable world state.
5. Add each verified row to the test-to-contract map before calling this catalog complete.

The [GrayFace patch reconciliation](MM7-GrayFace-Patch-Reconciliation.md) is a
public bug-index and behavior cross-check for choosing focused follow-ups; it
does not replace the current source, shipped data or tests as authority.

See also: [Campaign Status](Campaign-Status.md), [Current Delivery Order](Current-Delivery-Order.md), [MM7 GrayFace Patch Reconciliation](MM7-GrayFace-Patch-Reconciliation.md), and [MM7 Playtest Recording](MM7-Playtest-Recording.md).

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
