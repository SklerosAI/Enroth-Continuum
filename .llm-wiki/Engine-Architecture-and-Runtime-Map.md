---
type: concept
up: "[[Agent-Handoff]]"
tags: [architecture, mm6, runtime]
---

# Engine Architecture and Runtime Map

This is a source-linked map of the current Enroth fork for future agents; it identifies owners of behavior and known title seams, not a claim that MM6 gameplay works.

| Layer | Current source entry points | Current role and MM6 seam |
| --- | --- | --- |
| CLI and paths | [OpenEnrothOptions.cpp](../src/Bin/OpenEnroth/OpenEnrothOptions.cpp), [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp) | `--game mm6` or `mm7` selects a title; data-path validation and MM6 profile resolution are separate. Current CLI has no MM8 engine selection. Explicit data/user paths are safer than implicit shell state for a Finder launch. |
| App initialization | [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp), [Engine.cpp](../src/Engine/Engine.cpp) | Window, renderer, input, engine, game and scripting components start here. `Engine::Initialize` still enters `MM7_Initialize`, which calls `MM6_Initialize` and loads shared/MM7-oriented tables. Title branches exist for selected resources, tables and NPCs; title separation is incomplete. |
| Archives and assets | [ResourceManager.cpp](../src/Engine/Resources/ResourceManager.cpp), [EngineFileSystem.cpp](../src/Engine/Resources/EngineFileSystem.cpp), [LOD.cpp](../src/Engine/Resources/LOD.cpp) | MM6 tables/events are read from `icons.lod`; icons, bitmaps and sprites use title data archives. Retained private probes cover boundaries and references, but each runtime consumer still needs semantic checks. |
| World and snapshots | [Indoor.cpp](../src/Engine/Graphics/Indoor.cpp), [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp), [MM6OutdoorSnapshot.cpp](../src/Engine/Snapshots/MM6OutdoorSnapshot.cpp), [CompositeSnapshots.cpp](../src/Engine/Snapshots/CompositeSnapshots.cpp) | An MM6 outdoor base decoder exists. The [world closure packet](../project/MM6_WORLD_F2_CLOSURE.md) proves bounded retained delta section reassembly offline; the [field map](../project/MM6_WORLD_FIELD_MAP_F2.md) bounds records but corrects the chest delta's final 280 bytes to an unresolved grid/tail split. Indoor and C++ delta reconstruction, reset, persistent bytes, doors and chests still cross MM7 assumptions. |
| Rules and combat | [Character.cpp](../src/Engine/Objects/Character.cpp), [Party.cpp](../src/Engine/Party.cpp), [MM6ItemTable.cpp](../src/Engine/Tables/MM6ItemTable.cpp), [Spells.cpp](../src/Engine/Spells/Spells.cpp), [CastSpellInfo.cpp](../src/Engine/Spells/CastSpellInfo.cpp), [Monsters.cpp](../src/Engine/Objects/Monsters.cpp), [Actor.cpp](../src/Engine/Objects/Actor.cpp) | MM6 table loading is partial. [Creation audit](../project/MM6_CREATION_F2.md) finds starter-item collisions; [spell audit](../project/MM6_SPELL_EFFECT_F2.md) finds inherited effect/range/target assumptions; [combat packet](../project/MM6_COMBAT_SEMANTICS_F2.md) maps all retained monster rows to parser, identity, resistance, AI and loot consumers. No original combat behavior or encounter is verified. |
| Events and dialogue | [EvtProgram.cpp](../src/Engine/Evt/EvtProgram.cpp), [EvtInstruction.cpp](../src/Engine/Evt/EvtInstruction.cpp), [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp), [EvtVariables.cpp](../src/Engine/Evt/EvtVariables.cpp), [NPCTopics.cpp](../src/GUI/UI/NPCTopics.cpp) | Framing, operand decoding, execution, variable dispatch and NPC routing are separate obligations. The [opcode packet](../project/MM6_OPCODE_SEMANTICS_F2.md) structurally registers all 43 active values, but seven high-conflict operations and inherited MM7 variable dispatch need MM6 semantics. The [campaign packet](../project/MM6_CAMPAIGN_F2_CLOSURE.md) adds NPC, transport and finale seams; neither proves reachability. |
| UI and media | [UIPartyCreation.cpp](../src/GUI/UI/UIPartyCreation.cpp), [UIPopup.cpp](../src/GUI/UI/UIPopup.cpp), [UIHouses.cpp](../src/GUI/UI/UIHouses.cpp), [MediaPlayer.cpp](../src/Media/MediaPlayer.cpp), [AudioPlayer.cpp](../src/Media/Audio/AudioPlayer.cpp) | Party creation and potion UI retain MM7 assumptions. House type labels fall through the inherited parser; inherited portrait names miss the retained icon index and voice formulas have table gaps. See [media closure](../project/MM6_MEDIA_F2_CLOSURE.md). Correct original archives, title mappings, visible frames and audible playback remain unverified. |
| Save and profile | [SaveLoad.cpp](../src/Engine/SaveLoad.cpp), [UISaveLoad.cpp](../src/GUI/UI/UISaveLoad.cpp), [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp) | An MM6 profile root resolves, but serializers, filenames, load-menu filters and trace-save naming remain MM7-oriented. A versioned `.ec6` container and lossless map state are specified, not implemented. |
| Mac launcher | [AtlasApp.swift](../launcher/AtlasApp.swift), [Atlas HTML](../launcher/Atlas/index.html), [build.sh](../launcher/build.sh) | Atlas currently launches verified MM7 and detects MM6/MM8 data. `build.sh` copies one engine as `OpenEnroth.app`; a title-aware sibling assembler is required before adding another Play action. |

## Runtime path to verify

CLI or Atlas chooses a title and explicit paths → GameStarter validates the data and profile → Engine opens archives and initializes tables → Game loads a map and events → UI, rules, media and event interpreter act on that state → SaveLoad persists it. Each arrow is a separate check. Today MM7 has a verified graphical opening and save/reload baseline. MM6 reaches title/party screens only; an MM6 map session, save/reload and ending are not established. MM8 has no adapted engine.

The complete public MM6 gap register is [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). The [source/probe index](MM6-Source-and-Probe-Index.md) routes agents to focused evidence. Detailed proprietary-derived findings belong in the [private companion](Private-Companion-Wiki.md), never in this public page.

## Knowledge and evidence handoff

| Domain | Current strongest public evidence | Required next proof |
| --- | --- | --- |
| Original resources and guides | Retained indexes, bounded probes, and the [guide ledger](../project/MM6_GUIDE_F2_REVIEW.md) with chapter locators and selected visual checks. | Finish claim-level reconciliation, freeze F2, then run the single read-only F3 original-file preflight. |
| World and persistence | Python byte-exact delta section reassembly and [world field hypotheses](../project/MM6_WORLD_FIELD_MAP_F2.md). | Nonzero field meaning, MM6 C++ reconstruction and atomic `.ec6` save/reload, then graphical map-state checks. |
| Rules and combat | Static class/item/spell/monster consumer maps and [combat semantics audit](../project/MM6_COMBAT_SEMANTICS_F2.md). | Title-specific effects and formulas through exhaustive F4 offline tests and representative F5 encounters. |
| Events and campaign | All-active [opcode shape register](../project/MM6_OPCODE_SEMANTICS_F2.md), structural graph and campaign-route leads. | Exact MM6 side effects/variables and a visible, persistent route through milestones and ending. |
| UI, media and control | Retained name-index checks and source entry points; [agent-control footholds](Agent-Play-and-Observation-Roadmap.md). | Actual rendered frames and heard sound; a title-bound observe/act adapter satisfying the [future control contract](Agent-Play-Knowledge-and-Control-Contract.md). |

Each row is a different evidence level. The [agent knowledge/control contract](Agent-Play-Knowledge-and-Control-Contract.md) defines records for future direct play; it is a design obligation, not a current external API. The [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) is frozen for the read-only F3 pass and remains the scope authority.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Agent Play and Observation Roadmap](Agent-Play-and-Observation-Roadmap.md), [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md), [Runtime Isolation](Runtime-Isolation.md).
