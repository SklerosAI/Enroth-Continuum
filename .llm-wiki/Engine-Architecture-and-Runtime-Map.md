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
| World and snapshots | [Indoor.cpp](../src/Engine/Graphics/Indoor.cpp), [Outdoor.cpp](../src/Engine/Graphics/Outdoor.cpp), [MM6OutdoorSnapshot.cpp](../src/Engine/Snapshots/MM6OutdoorSnapshot.cpp), [CompositeSnapshots.cpp](../src/Engine/Snapshots/CompositeSnapshots.cpp) | An MM6 outdoor base decoder exists. Indoor and map-delta reconstruction, reset, persistent bytes, doors and chests still cross MM7 snapshot assumptions. See [save spec](../project/MM6_SAVE_SPEC.md). |
| Rules and combat | [Character.cpp](../src/Engine/Objects/Character.cpp), [Party.cpp](../src/Engine/Party.cpp), [MM6ItemTable.cpp](../src/Engine/Tables/MM6ItemTable.cpp), [Monsters.cpp](../src/Engine/Objects/Monsters.cpp), [Actor.cpp](../src/Engine/Objects/Actor.cpp) | MM6 table loading is partial; class, item, spell, potion, monster labels, resistance, hostility and drops need MM6-specific meaning. A parsed row is not a rule check. |
| Events and dialogue | [EvtProgram.cpp](../src/Engine/Evt/EvtProgram.cpp), [EvtInstruction.cpp](../src/Engine/Evt/EvtInstruction.cpp), [EvtInterpreter.cpp](../src/Engine/Evt/EvtInterpreter.cpp), [EvtVariables.cpp](../src/Engine/Evt/EvtVariables.cpp), [NPCTopics.cpp](../src/GUI/UI/NPCTopics.cpp) | Framing, operand decoding, execution, variable dispatch and NPC routing are separate obligations. Current MM6 Question and other active instructions need title-specific behavior; the structural graph is not campaign reachability. |
| UI and media | [UIPartyCreation.cpp](../src/GUI/UI/UIPartyCreation.cpp), [UIPopup.cpp](../src/GUI/UI/UIPopup.cpp), [UIHouses.cpp](../src/GUI/UI/UIHouses.cpp), [MediaPlayer.cpp](../src/Media/MediaPlayer.cpp), [AudioPlayer.cpp](../src/Media/Audio/AudioPlayer.cpp) | Party creation and potion UI retain MM7 assumptions. House/movie/sound mappings require MM6 routing; indexed files do not prove video or audio playback. |
| Save and profile | [SaveLoad.cpp](../src/Engine/SaveLoad.cpp), [UISaveLoad.cpp](../src/GUI/UI/UISaveLoad.cpp), [PathResolver.cpp](../src/Application/Startup/PathResolver.cpp) | An MM6 profile root resolves, but serializers, filenames, load-menu filters and trace-save naming remain MM7-oriented. A versioned `.ec6` container and lossless map state are specified, not implemented. |
| Mac launcher | [AtlasApp.swift](../launcher/AtlasApp.swift), [Atlas HTML](../launcher/Atlas/index.html), [build.sh](../launcher/build.sh) | Atlas currently launches verified MM7 and detects MM6/MM8 data. `build.sh` copies one engine as `OpenEnroth.app`; a title-aware sibling assembler is required before adding another Play action. |

## Runtime path to verify

CLI or Atlas chooses a title and explicit paths → GameStarter validates the data and profile → Engine opens archives and initializes tables → Game loads a map and events → UI, rules, media and event interpreter act on that state → SaveLoad persists it. Each arrow is a separate check. Today MM7 has a verified graphical opening and save/reload baseline. MM6 reaches title/party screens only; an MM6 map session, save/reload and ending are not established. MM8 has no adapted engine.

The complete public MM6 gap register is [MM6 implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). The [source/probe index](MM6-Source-and-Probe-Index.md) routes agents to focused evidence. Detailed proprietary-derived findings belong in the [private companion](Private-Companion-Wiki.md), never in this public page.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Agent Play and Observation Roadmap](Agent-Play-and-Observation-Roadmap.md), [Runtime Isolation](Runtime-Isolation.md).
