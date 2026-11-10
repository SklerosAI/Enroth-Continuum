---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm7, scrolls, gameplay]
---

# MM7 Scroll Cancellation

Item-, actor- and character-target spell scrolls now return to the cursor when the player cancels their target pick in MM7.

The [integrated change](../src/Engine/Objects/Character.cpp) keeps an item-target scroll with the pending cast. Actor- and character-target scrolls stay on the cursor until the queued cast starts. The Escape path in [Game.cpp](../src/Application/Game.cpp) returns a targeted scroll if the player cancels before choosing a target. A completed cast consumes one scroll. A general spell cancellation also occurs during Load, so that path does not refund.

The [GameTests](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp) check Fire Aura, Fire Bolt and Cure Weakness cancellation and completion, plus an actor-target scroll read from the open inventory. On integrated source, the four scroll cases, 11 existing spell/scroll cases and the Light route passed 16/16 headlessly. A marked disposable production Save writer and separate normal-Load reader passed 1/1 each with unchanged slot bytes after the shared engine change. Unit tests passed 1,087 with 45 skipped. See the [item-target handoff](../agent_comms/handoffs/2026-11-10-claude-mm7-scroll-cancel.md) and [actor/character-target handoff](../agent_comms/handoffs/2026-11-10-claude-mm7-actor-scroll-cancel.md) for branch checks and revert-and-fail evidence.

Other targeting exits, including attacking, passing the turn, stealing, the caster becoming unable to act, a full spell queue and turn-based wait, remain in the audit queue. No visible game run has checked the installed Atlas since the latest headless installation.

See also: [Campaign Status](Campaign-Status.md), [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [Agent Handoff](Agent-Handoff.md).
