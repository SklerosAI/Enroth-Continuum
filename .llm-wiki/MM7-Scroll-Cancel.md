---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm7, scrolls, gameplay]
---

# MM7 Scroll Cancellation

Item-target spell scrolls now return to the cursor when the player cancels the item pick in MM7.

The [integrated change](../src/Engine/Objects/Character.cpp) keeps the scroll with the pending cast after it leaves the cursor. The Escape path in [Game.cpp](../src/Application/Game.cpp) returns that item if the item pick is cancelled. Selecting a target consumes one scroll. The refund is confined to this casting path because a general spell cancellation also occurs during Load.

The [GameTest](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp) checks Fire Aura cancellation and completion, including the total scroll count, cursor item, screen and sword enchantment. On integrated source, that case and 11 existing spell/scroll cases plus the Light route passed 13/13 headlessly. A marked disposable production Save writer and separate normal-Load reader passed 1/1 each with unchanged slot bytes after the shared engine change. Claude's branch also passed the full headless MM7 GameTest suite and unit suite before integration; see [the handoff](../agent_comms/handoffs/2026-11-10-claude-mm7-scroll-cancel.md) for counts and revert-and-fail evidence.

Actor- and character-target scroll cancellation remains a separate open case. No visible game run has checked the installed Atlas since the latest headless installation.

See also: [Campaign Status](Campaign-Status.md), [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [Agent Handoff](Agent-Handoff.md).
