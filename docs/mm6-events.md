# MM6 events

How this port reads and runs MM6 event scripts (`*.evt` and `global.evt`) through the shared interpreter in
`src/Engine/Evt`, and where MM6 differs from MM7.

## Where the code lives

MM6 has no interpreter of its own. `EvtProgram::load` and `EvtInstruction::parse` take a `GameTitle`, and
`EvtInterpreter` branches on `engine->gameTitle() == GameTitle::MM6` per opcode. The MM6 helpers next to it in
`src/Engine/Evt/MM6*.{h,cpp}` hold the title rules (variable families, item, gold, quest bits, NPC state). Each helper
throws on an operand outside the range that shipped MM6 scripts use, rather than falling into MM7 meanings.

`Engine.cpp` loads `global.evt` and the current map's `<map>.evt` with the running title. A parse failure names the
byte offset and event id. The unit test `MM6EventFraming.AllActiveShippedScripts` parses all 68 active MM6 scripts (67
map scripts plus `global.evt`) when the private game data is available.

## Binary format

A file is a flat list of records. Each record is:

| Offset | Size | Field |
|---|---|---|
| 0 | u8 | Length of the rest of the record, so the record is `length + 1` bytes |
| 1 | u16 | Event id |
| 3 | u8 | Step within the event |
| 4 | u8 | Opcode |
| 5 | var | Payload |

The frame is the same in MM6 and MM7. Payloads differ for several opcodes, so the parser switches on title. The parser
requires the payload to be consumed exactly, so a layout mismatch fails at load time instead of mis-running later.

MM6 scripts use only opcodes 1 to 43. Opcodes 44 and up (the dialogue-item checks, actor group commands, date timers
and the MM8 additions) never occur in shipped MM6 data.

## MM6 payloads that differ from MM7

- **5 `LocationName`** (MM6 only): a hint record with a local string id. Its step is forced to -1, like `MouseOver`,
  so it never runs.
- **9 `ReceiveDamage`**: the damage kind byte is numbered like the `monsters.txt` labels: Physical, Magic, Fire,
  Electricity, Cold, Poison, Energy (`mm6::damageTypeForEventKind`). Kinds past Energy deal Physical.
- **11 `SetTexture`**: the first operand is an indoor face index, not a cog number. Outdoors or out of range the command
  does nothing, as in the original.
- **12, `ShowMovie` in MM7**: MM6 sets an outdoor face texture, `model:u32, facet:u32, name`, both indices zero-based.
  No movie plays.
- **13 `SetSprite`**: the first operand is the zero-based index into the map's decoration array, since MM6 sprites have
  no cog. The byte is a visibility flag. A name other than empty or `"0"` replaces the decoration (`MM6SpriteEvent.h`).
- **14, 16, 17, 18 `Compare`, `Add`, `Subtract`, `Set`**: the variable id is one byte in MM6 and two in MM7. The ids
  mean different things, see below.
- **19 `SummonMonsters`**: 15 bytes, encounter slot (1 to 3), tier (1 to 3 for A, B, C), count, x, y, z. No group or
  unique-name fields.
- **20, 27, 28** (MM6 only): payloads of 5, 2 and 1 bytes. The original dispatcher steps straight to the next record
  without reading them, so the port keeps the bytes and advances. Editor tools label them ModifyItem, RndPassword and
  RndAnswer, which is not an effect.
- **23 `SetFacesBit`**: indoor face index plus attribute mask and on/off, applied to that one face. Same no-op rule as
  11.
- **24, `ToggleActorFlag` in MM7**: MM6 sets or clears an attribute mask on outdoor faces,
  `model:u32, facet:u32, bit:u32, on:u8`. Facet `0xFFFFFFFF` means every face of the model.
- **26, `InputString` in MM7**: MM6 Question, three local string ids (question, answer 1, answer 2) and a target step.
- **33 `PressAnyKey`** (MM6 only): opens the branchless dialogue and resumes at the next step.
- **69 `RefundChestArtifacts`** (MM6 only, a GrayFace patch addition): one byte, a chest index. No shipped MM6 script
  uses it, GrayFace's patched `d18`, `hive`, `outb2` and `pyramid` scripts do. MM7 still refuses the record.

Outdoor face targets resolve through `pOutdoor->pBModels[model].faces`. An out-of-range model or facet throws, since
every shipped record is in range.

## Opcode behaviour notes

- **2 `SpeakInHouse`**: enters the house through the MM6 house UI. MM6 plays no house-enter sound here.
- **6 `MoveToMap`**: an all-zero position means "don't place the party". On the current map the party stays put
  (castle throne rooms open this way), on another map it arrives at Party Start. Shipped MM6 doors name only the map.
- **15 `ChangeDoorState`**: shared door code. Action 3, GrayFace's "switch even while moving", runs through
  `mm6::switchDoorEvenIfMoving` behind `fix_door_state_switch`. No shipped MM6 or MM7 script passes it.
- **21 `CastSpell`**: only verified event spells run, see below.
- **26 Question**: `startMM6QuestionDialogue` shows the question and takes typed text. Either answer, compared ignoring
  ASCII case, jumps to the target step. Other text resumes at the next step. Escape stops the event.
- **39 `SetNPCTopic`**: `mm6::setNpcTopic`, slots 0 to 2 only, event id 0 removes the topic. The MM7 special case for
  NPC 8 is skipped.
- **40 `MoveNPC`**: `mm6::moveNpc`, house 0 removes the NPC from any house. A move can stop the current house's room
  movie loop.
- **41 `GiveItem`**: `mm6::giveEventItem` rolls a random item on the MM6 generator even when a fixed id is given. The
  fixed id replaces only the item number, and the rolled bonus, charges and identified flag stay. The item goes to the
  cursor. A held item moves to the active character, then to characters 0 to 3.
- **42 `ChangeEvent`**: rebinds a decoration's click event. MM6 stores it relative to base 400
  (`MM6_DECORATION_EVENT_BASE`), MM7 to 380. Operand 0 hides the decoration.
- **43 `CheckSkill`**: encoded mastery 0 means "any trained mastery" and is checked as a minimum level. Other masteries
  match exactly (`mm6CheckSkillSatisfied` in `MM6EventRules.h`).
- **69 `RefundChestArtifacts`**: takes the artifacts that chest's generation put on the found list, and that weren't
  in its grid when it was first opened, off the list again (`MM6ChestArtifactRefund.h`). They stay in the chest. It
  runs whatever `fix_unopened_chest_artifacts` says.

### Event spells

`mm6::castEventSpell` (`src/Engine/MM6/MM6EventSpell.h`) runs the spells shipped scripts cast. `isVerifiedEventSpell`
in `MM6SpellEffectGuard.h` is the allow list. Anything else throws.

- Projectiles fly at the target point: Flame Arrow (2), Static Charge (13), Sparks (15), Cold Beam (24), Magic Arrow
  (35), Rock Blast (41), and ids 6, 28, 32 and 90 from the damage table (`isEventProjectileSpell`). Flame Arrow and
  Magic Arrow fly, sound and hurt as Cold Beam, which is what MM6.exe does. Sparks fans 3, 5 or 7 sparks over 60
  degrees by mastery.
- Damage comes from the original event damage table in `eventSpellDamage`. Only the low six bits of the event's skill
  level count, so level 100 hits as 36.
- Bless (46) blesses every character with no power, 240 s plus 300 s per level at Normal and 900 s per level above.
- Day of the Gods (83) raises all seven stats on every character by (mastery + 1) times level, for level hours.
- Mass Curse (91) tries to curse every character, but only when the party is near the launch point.
- Wizard Eye (12) and Armageddon (98) do nothing from an event, as in the original.

## Target character

Commands that act on characters use the selector set by `ForPartyMember` (35): one member, the active member, the whole
party or a random member. `EvtInterpreter::iterateCharacters` resolves it.

MM6 lets a click or Space on an event face or decoration run with nobody active (`canRunInteractionEvent` in
`Interaction.cpp`). MM7 refuses. With nobody active, vanilla MM6.exe picks a fresh random member for every command, so a
check could read one member and the reward land on another. With `gameplay.fix_inactive_party_events` (default on) the
port does what the GrayFace patch does: one member who can act is picked on the first command that needs it and kept
for the whole run, falling back to member 1. A run that resumes after a dialogue picks again.

## Event variables

MM6 variable ids are one byte and are not MM7's `EvtVariable` values, even where numbers collide.
`describeMM6Variable` in `MM6EventVariables.h` sorts an id into a family, and the interpreter dispatches MM6 families
before the shared executor. Only family `SharedCandidate` (ids 1 to 45) reaches the inherited `EvtVariables.cpp`, after
MM6 value-range checks in `isValidMM6SharedCandidateValue`. Any other id with no MM6 executor throws.

| Id | Family | Code |
|---:|---|---|
| 1 to 45 | Shared (stats, class, level, award and similar) | `EvtVariables.cpp` |
| 12 | Award, on Set and Add | `MM6EventVariableDispatch.cpp` |
| 16 | Quest bit | `MM6QuestBitState` |
| 17 | Item | `MM6ItemEventCompare`, `MM6ItemEventWrite` |
| 19 | Day of year | `MM6DayOfYear` |
| 21 | Gold | `MM6GoldEvent` |
| 46 to 55 | Resistance | `MM6ResistanceEvent` |
| 56 to 86 | Skill | `MM6SkillEvent` |
| 87 to 104 | Condition | `MM6ConditionVariables` |
| 105 to 204 | Map byte | `MM6MapByteState` |
| 205 | Autonote | `MM6AutonoteState` |
| 213 | Character bit | `MM6CharacterEventBit` |
| 214 | Hired NPC | `MM6HirelingEvent` |
| 215 | Reputation | `MM6ReputationEvent` |
| 216 to 221 | Day counter | `MM6DayCounterEvent` |
| 222 | Flying | `MM6PartyFlying` |
| 223 | Hired profession | `MM6HirelingEvent` |
| 224 | Circus prizes | `MM6CircusPrizeEvent` |
| 225 | Skill points | `MM6EventVariableDispatch.cpp` |
| 226 | Month | `MM6MonthOfYear` |

Rules per family:

- **Shared (1 to 45)**: through the inherited executor. Class takes the 18 MM6 classes, award rows 1 to 100.
- **Award (12)**: Set and Add give the award and play the award reaction every time, even for an award the character
  has.
- **Quest bit (16)**: a party quest bit. A bit with no quest text is still state, shipped scripts use those too.
- **Item (17)**: ids 1 to 580. Compare passes when any selected character has the item in the backpack or it is on the
  cursor. Add puts an identified plain item on the cursor, moving the held item to a backpack first. Subtract removes
  the first match per selected character in row-major grid order, then the cursor. Set throws.
- **Day of year (19)**: one-based day in a 336-day year. Compare only.
- **Gold (21)**: party gold, applied once. A Subtract that can't be paid stops the event. Set throws.
- **Resistance (46 to 55)**: five resistances (Fire, Electricity, Cold, Poison, Magic) then their five bonuses. Set
  stores the low byte, Add caps at 255, Subtract doesn't floor. Writes play a reaction.
- **Skill (56 to 86)**: the 31 MM6 skill slots in MM6 order. The last slot is Learning, which sits elsewhere in the
  shared enum. Compare and Set only.
- **Condition (87 to 104)**: 17 conditions in MM6 order, then Good (104), which skips MM7's Zombie slot. No operation
  reads its operand. Set and Add start the condition with no protection check, and Good clears all. Subtract clears it.
- **Map byte (105 to 204)**: 100 per-map bytes in the map delta (`.ddm` or `.dlv`). Only 105 to 175 occur in shipped
  scripts and only those are accepted. As in MM6.exe's handlers (Compare 0x43FE40, Set 0x4407E0, Add 0x4412B0,
  Subtract 0x4420E0), Compare is "at least" against the whole signed operand, Set stores the operand's low byte, Add
  adds the low byte and stops at 255, and Subtract takes the low byte away and wraps below 0. No other family shares
  this code. MM6.exe runs a write once per targeted character, so a whole-party Add lands four times. The port writes
  once. Only `zddb05` event 6 and `zddb07` event 10 add for the whole party, each a once-only flag that is only
  compared with 1, on maps no script leads to. Every shipped Subtract is guarded by a Compare with its operand, so none
  wraps. Repeated clicks push an Add past 255 on three maps: the Temple of the Moon (`t5`) altars add 1 to 6 per touch
  once their trap is sprung, the Temple of Tsantsa (`t3`) levers add 1 per pull, and the Icewind Keep (`d15`) chest
  adds 1 per opening once an opening with quest bit 25 clear has started the count. `zddb10` has unbounded adds too,
  on a map no script leads to. Test `MM6Dungeons.MoonAltarTouchedPast255Times`.
- **Autonote (205)**: bits 1 to 128 of the party autonote field.
- **Character bit (213)**: 512 bits per character. The wire index is zero-based (MM6.exe tests bit `0x80 >> (n % 8)` of
  byte `n / 8`), the runtime bitset is one-based. Compare and Set only.
- **Hired NPC (214)**: Compare passes when that NPC table entry travels with the party. Set and Add hire, Subtract
  dismisses.
- **Reputation (215)**: Compare is equality against effective reputation, where a hired Bard adds 200 and each hired
  Pirate, Gypsy, Duper or Burglar takes 200 away. A negative operand always passes. Writes are unclamped and run once
  per targeted character, so a whole-party write lands four times, as in MM6.
- **Day counter (216 to 221)**: six counters. Set and Add restart at now, Subtract clears. Compare passes when at least
  that many calendar days passed since the restart.
- **Flying (222)**: Compare only. True when the party flies and the Fly buff is active. The operand is ignored, as in
  MM6.exe.
- **Hired profession (223)**: Compare passes when some hireling has the profession. Subtract dismisses every hireling
  with it.
- **Circus prizes (224)**: Compare passes when the prize score is at least the operand. Items 470, 471 and 477 count 1,
  3 and 5, equipped items included, the cursor excluded.
- **Skill points (225)**: per character. Subtract floors at zero.
- **Month (226)**: Compare only, zero-based month against a zero-based operand.

Ids 206 to 212 and 227 and up have no MM6 executor and throw.

Writes to awards, resistances, conditions and skills play the character's award sound and, where MM6 has one, a
portrait reaction. MM6 also sparkles over the portrait, which the shared buff animation can't draw for MM6.

All of these states are saved in the EC6 container. Map bytes travel with the map delta, the rest with party and
character fields. See [MM6 save format](mm6-save-format.md).

## Triggers and timers

MM6 map scripts declare triggers with `OnTimer` (31), `OnLongTimer` (38) and `OnMapReload` (37). The trigger
instruction itself stops execution if reached. `Processor.cpp` registers them on map load and runs the steps after
them.

A timer has either a half-minute interval or a period (yearly, 28-day month, weekly, or daily at a time of day). The
map's last visit time comes from the 56-byte visit block at the end of the map delta (`mm6::mapLastVisit` in
`MM6MapVisit.h`). In MM6.exe only an `OnLongTimer` without a half-minute interval can fire at once on entry, when the
map was never left or was left a day or more ago. Every other timer gets a calendar alarm counted from now. A daily
timer's hour byte is used as is, so an hour byte past 23 means that many hours after the next midnight
(`mm6::mapTimerFirstAlarm`).

Vanilla MM6 restarts the countdown on every visit, so a fountain left just before its refill waits another whole period.
With `gameplay.fix_map_timers` (default on) the port does what the GrayFace patch does. Each map's last `OnLongTimer`
firing per period kind is kept in `mm6::mapTimerFirings()`, saved in the optional EC6 `TIMR` section
(`MM6MapTimerSection`), and the next alarm counts from it. A daily timer whose time hasn't come yet today fires today. A
late revisit fires a missed periodic timer once, not once per missed period.

## NPC events and topics

NPC topics in MM6 are `global.evt` functions. `mm6::npcTopicRoute` (`MM6NpcEventState.h`) runs the global event when
one exists for the topic id. MM6 topic ids overlap MM7's numeric ranges for teachers, guild offers and the arena, so the
MM7 special handlers are skipped in MM6. A topic with no global function logs an error and returns to the main
dialogue.

Some topics have no script because MM6.exe answers them itself: guild membership (381 to 397), expert and master
teachers (200 to 259) and the town hall bounty (399). `mm6SelectHouseTopic` handles those inside the MM6 house window,
see [houses and services](mm6-houses-and-services.md). `SpeakNPC` (22) opens an NPC dialogue, or during map load records the NPC for later.

`StatusText` (29) and `ShowMessage` (30) read local strings for map events and NPC topic text for global events.

## Facets, sprites and houses

- Indoor faces: opcodes 11 and 23 address one face by global index. MM7 matches every face with a cog number, which
  can hit several faces or none for an MM6 index.
- Pressure plates: MM6 also runs the event of a wall or ceiling plate the party pushes against, every frame. See
  [travel and maps](mm6-travel-and-maps.md#doors-and-indoor-faces).
- Outdoor faces: opcodes 12 and 24 address model and facet.
- Decorations: 13 by decoration index, 42 rebinds the click event. Clicking an interactive decoration runs global event
  `decorVars[eventVarId] + 400`.
- Houses: 2 enters a house. `MoveToMap` (6) can carry a house id and exit picture. See
  [houses and services](mm6-houses-and-services.md) and [travel and maps](mm6-travel-and-maps.md).

## Unsupported and partial

- Spells outside the verified list throw, see above.
- Variable ids 206 to 212 and 227 and up throw. Item Set, gold Set and day-of-year writes throw. A whole-party item
  Add throws.
- `GiveItem` throws when no character has room for the held item. MM6 drops it on the ground, which the port doesn't do.
- The generated-artifact item band is refused by item Add.
- `ChangeDoorState` action 3 is not implemented. It only matters for GrayFace-patched scripts.
- With nobody active, the stand-in member of `fix_inactive_party_events` steers only the character selector. Where an
  event moves a held item while giving a new one, that path still sees nobody active.

## Source

- [EvtInstruction.cpp](../src/Engine/Evt/EvtInstruction.cpp), [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp),
  [EvtProgram.cpp](../src/Engine/Evt/EvtProgram.cpp), [EvtEnums.h](../src/Engine/Evt/EvtEnums.h),
  [Processor.cpp](../src/Engine/Evt/Processor.cpp)
- [MM6EventVariables.h](../src/Engine/Evt/MM6EventVariables.h),
  [MM6EventVariableDispatch.h](../src/Engine/Evt/MM6EventVariableDispatch.h),
  [MM6EventRules.h](../src/Engine/Evt/MM6EventRules.h), [MM6NpcEventState.h](../src/Engine/Evt/MM6NpcEventState.h),
  [MM6SpriteEvent.h](../src/Engine/Evt/MM6SpriteEvent.h), [MM6GiveItemEvent.h](../src/Engine/Evt/MM6GiveItemEvent.h)
- [MM6EventSpell.h](../src/Engine/MM6/MM6EventSpell.h), [MM6SpellEffectGuard.h](../src/Engine/MM6/MM6SpellEffectGuard.h),
  [MM6MapVisit.h](../src/Engine/MM6/MM6MapVisit.h), [MM6MapTimerSection.h](../src/Engine/MM6/MM6MapTimerSection.h)
- [Interaction.cpp](../src/Engine/Interaction.cpp), [UIBranchlessDialogue.cpp](../src/GUI/UI/UIBranchlessDialogue.cpp),
  [NPCTopics.cpp](../src/GUI/UI/NPCTopics.cpp)
- Unit tests: [src/Engine/Evt/Tests](../src/Engine/Evt/Tests) (`MM6EventFraming_ut.cpp` and one file per helper)
- Game tests: `GameTests_MM6EventVariables.cpp`, `GameTests_MM6Questions.cpp`, `GameTests_MM6OriginalNoopEvent.cpp`,
  `GameTests_MM6CharacterBitEvent.cpp`, `GameTests_MM6MonthEvent.cpp`, `GameTests_MM6FlyingEvent.cpp`,
  `GameTests_MM6GiveItemEvent.cpp`, `GameTests_MM6SkillEvent.cpp`, `GameTests_MM6MapVisit.cpp` in
  [test/Bin/GameTest](../test/Bin/GameTest)

See also: [Port architecture](mm6-port-architecture.md), [Save format](mm6-save-format.md), [Houses](mm6-houses-and-services.md), [Travel and maps](mm6-travel-and-maps.md)
