---
type: concept
up: "[[Agent-Play-and-Observation-Roadmap]]"
tags: [agent-control, knowledge, observation, validation]
---

# Agent Play Knowledge and Control Contract

This is a **future interface and evidence contract**, not a claim that an agent can play MM6 today. Implement it only after that title has passed its engine, graphical and save/reload gates in the [plan](../project/plan.md). The source-linked [runtime map](Engine-Architecture-and-Runtime-Map.md), [F2 findings](MM6-F2-Structural-Findings.md), and [source/probe index](MM6-Source-and-Probe-Index.md) describe what is currently known. Exact game-derived records, screenshots, audio, traces and saves stay in the ignored [private companion](Private-Companion-Wiki.md).

## Claim and knowledge record

Each rule or route an agent may rely on needs one durable record with these fields. Public wiki entries contain safe aggregates; the private companion supplies exact game-derived values and locators.

| Field | Required content |
| --- | --- |
| Identity | Stable claim key, title (`mm6`, `mm7`, or `mm8`), subsystem, scenario and revision date. |
| Provenance | Tracked spec/probe/source commit SHA and function; local-only retained report or original-resource fingerprint; guide page locator when used. |
| Claim | One testable assertion, expected visible or saved result, confidence, competing interpretations and superseded claims. |
| Evidence tier | **Inventory** (present/indexed), **parse** (bounded decoding), **semantic** (meaning reconciled against sources), **offline implementation** (executed title-specific behavior), **runtime** (visible/heard result), or **campaign** (progression plus durable save/reload). A higher tier needs its own evidence; parser success cannot promote a rule to runtime. |
| Consumer and closure | C++/UI/save/media consumer, current gap, acceptance scenario, negative case, next gate, and private artifact reference. |

At the current F2 gate, many MM6 claims stop at inventory, parse or source-level semantic leads. The [F2 gate review](../project/MM6_F2_GATE_REVIEW.md) is open. A future play agent must load only the title-specific, source-backed knowledge it needs for the current scenario, prefer fresh runtime observations over an old plan, and stop on a contradiction instead of silently inventing a rule.

## Session identity and observation

Before the first action, a controller must bind a session to **one** title, engine build, installed helper, owned-data fingerprint kept locally, and isolated profile/save namespace. A matching app window and an actual rendered frame must confirm the selected title. Reject a mismatched title, stale frame, unfocused window or unverified helper/profile before sending input. The observation record should contain:

| Record field | Meaning and check |
| --- | --- |
| Session | Opaque session ID, selected title, build SHA, helper identity, profile token and local save checkpoint reference. Do not expose a private absolute path in public output. |
| Visible frame | Monotonic sequence/time, focused window identity, viewport size and coordinate transform, local frame reference/hash, and whether the frame is blank, obscured or unchanged after an action. A save thumbnail alone is insufficient. |
| Audible state | Separate local playback/recording or human-observed result with device/volume context; an indexed sound name or open audio device is not heard playback. |
| Game checkpoint | Visible location/menu/dialogue/party state, expected transition or action outcome, uncertainty, and local save/trace reference. Do not treat a log line or hidden engine variable as a substitute for the view. |

The current [OpenGL renderer](../src/Engine/Graphics/Renderer/OpenGLRenderer.cpp) has a viewport screenshot method used for thumbnails, while [RendererBindings.cpp](../src/Scripting/RendererBindings.cpp) does not expose a frame-capture API. [EngineController.cpp](../src/Engine/Components/Control/EngineController.cpp) can post internal key/mouse events, but there is no reviewed external control surface. These are implementation footholds only. A native visible-window route may serve as the first observation/control adapter if it can meet the same identity, focus and checkpoint checks.

## Bounded action and acknowledgment

An action request should carry `session_id`, expected title and frame sequence, one user-equivalent key or mouse action, viewport-relative coordinates when relevant, maximum duration, and a unique action key. The adapter must reject mismatched or stale requests, clicks outside the verified viewport, action floods and cross-title helper/profile changes. It should return whether the action was accepted and executed, the before/after frame sequences, elapsed time, visible change or timeout, and a local diagnostic reference. There is no write access to hidden party, quest, map or inventory state through this contract.

The controller runs **observe → choose one bounded action → acknowledge → observe again**. It may use source knowledge to predict a result, but the next visible frame and saved-state check decide whether the action worked. Save/quit/relaunch/load is a required checkpoint for progress claims. On ambiguous focus, blank frames, unexpected dialog, repeated unchanged frames, wrong title, save failure or contradiction with a recorded rule, stop input, retain the last known-good save and record the discrepancy for a human/integrator review. Do not replay a possibly executed action blindly.

## Acceptance before unattended play

1. Establish a human-visible title-specific baseline: MM6 needs completed F2/F3/F4 and F5 checks; MM7 has a limited opening/save baseline but its full campaign audit is open; MM8 has no adapted runtime.
2. Validate window/frame identity and keyboard/mouse focus at the installed Mac app. Test menu, movement, inventory, dialogue, turn mode, save and load with visible before/after frames. Verify audio separately.
3. Validate failure cases: wrong title, stale frame, lost focus, coordinate mismatch, hung action, crash, truncated save and cross-title profile. None may mutate another title or overwrite the last good checkpoint.
4. Run a recoverable visible session through representative combat, house, travel, quest and map transitions, with save/reload checks. Full-campaign and ending claims require their own observed route; a scripted trace or static graph is not enough.

Keep the optional controller disabled during ordinary play, title-scoped when enabled, and compatible with the user's separate MM6/MM7/MM8 helpers and profiles. The existing [trace recorder/player](../src/Engine/Components/Trace/EngineTraceRecorder.cpp) and [game trace handler](../src/Application/GameTraceHandler.cpp) may help reproducibility, but its MM7 save naming requires review before MM6 use. Detailed observation logs belong under `local-game-data/private-wiki/` or another ignored local-only location and need an owner-approved backup because Git does not preserve them.

See also: [Agent Play and Observation Roadmap](Agent-Play-and-Observation-Roadmap.md), [Agent Handoff](Agent-Handoff.md), [Engine Architecture and Runtime Map](Engine-Architecture-and-Runtime-Map.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
