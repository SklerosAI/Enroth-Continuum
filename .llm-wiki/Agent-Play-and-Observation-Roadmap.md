---
type: concept
up: "[[Agent-Handoff]]"
tags: [agent-control, observation, validation]
---

# Agent Play and Observation Roadmap

Future agents should be able to observe and play a verified Enroth campaign on this Mac through a controlled, visible interface; no current MM6 agent-play capability is claimed here.

The [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md) defines the future claim record, session binding, visible observation, bounded action, acknowledgment, failure recovery and acceptance checks. It is a design contract, not an implemented bridge.

## Existing technical footholds

- [GameStarter.cpp](../src/Application/Startup/GameStarter.cpp) creates the window, renderer, input, engine and game components. [EngineController.cpp](../src/Engine/Components/Control/EngineController.cpp) can post keyboard and mouse events from a control routine. Those internal methods are not a complete, safe external agent API.
- [OpenGLRenderer.cpp](../src/Engine/Graphics/Renderer/OpenGLRenderer.cpp) implements a viewport screenshot method used for save thumbnails. [RendererBindings.cpp](../src/Scripting/RendererBindings.cpp) currently exposes shader reload, not frame capture. A screenshot method in source is not proof that an agent can inspect the installed game.
- [EngineTraceRecorder](../src/Engine/Components/Trace/EngineTraceRecorder.cpp), [EngineTracePlayer](../src/Engine/Components/Trace/EngineTracePlayer.cpp), and [GameTraceHandler.cpp](../src/Application/GameTraceHandler.cpp) support trace recording/replay tied to a save. The handler still names its trace save with an MM7 extension. Traces are deterministic regression tools, not evidence of an MM6 playthrough or a title-safe public agent bridge.
- [AtlasApp.swift](../launcher/AtlasApp.swift) currently launches MM7 from a local Mac app and detects MM6/MM8 data. It has no MM6/MM8 Play action yet.

## Staged path to direct play

| Stage | Required result | Gate and evidence |
| --- | --- | --- |
| 1. Human-visible MM6 baseline | Finish F2, perform F3, complete F4, then pass F5 with actual opening, map, combat, quest and save/reload checkpoints. | [Plan §18.1](../project/plan.md) and [MM6 checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md); do not use a partial title screen as a control target. |
| 2. Observe the rendered game | Capture the active title's visible frame/window, title identity, viewport dimensions, timestamp and focus state, and inspect graphical changes after an action. Keep images and recordings local. | Compare screenshots with on-screen behavior and a saved/reloaded checkpoint. Detect blank frames, wrong title/data, missing portraits, menu failures and visual stalls. Audio requires a separate heard/playback check. |
| 3. Bounded input | Use native UI control or a reviewed bridge that can send only user-equivalent key/mouse actions to the focused, selected title. Confirm coordinates, focus, action acknowledgment, timeout and escape/recovery behavior. | Verify move, click, menu, dialogue, turn mode and save actions on a staged app before unattended play. Do not expose hidden engine state or mutate party/quest fields to simulate play. |
| 4. Repeatable agent session | Run a visible observe → decide → act → observe loop with checkpoints, local screenshots, private logs and recoverable saves. Add title-specific trace/replay support if useful. | Save, quit, relaunch and load; verify state and image consistency, then progress through representative regions and an ending. A failed step stops safely without overwriting the last known-good save or another title's profile. |

MM7's verified Emerald Island launch is a useful graphical control, but its full campaign audit is still open. MM6 and MM8 remain unavailable for direct agent play until their own gates pass. Keep the control surface title-scoped: the MM6 agent cannot command MM7 or MM8 by changing data paths or helper slots. Any future Atlas bridge must retain the original Vanilla route and independent profiles.

The private companion at `local-game-data/private-wiki/INDEX.md` can hold local observation notes, frame references, focus/input calibration and play checkpoints. Public wiki pages record only nonproprietary conclusions and evidence levels. The prior [plan's optional agent bridge](../project/plan.md) also requires normal play to remain unchanged when that bridge is off.

See also: [Agent Play Knowledge and Control Contract](Agent-Play-Knowledge-and-Control-Contract.md), [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [Private Companion Wiki](Private-Companion-Wiki.md), [Campaign Status](Campaign-Status.md), [Agent Handoff](Agent-Handoff.md).
