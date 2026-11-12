---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm7, scrolls, gameplay]
---

# MM7 Scroll Cancellation

Item-, actor- and character-target spell scrolls now return to the cursor when the player cancels their target pick in MM7.

The [integrated change](../src/Engine/Objects/Character.cpp) keeps an item-target scroll with the pending cast. Actor- and character-target scrolls stay on the cursor until the queued cast starts. The Escape path in [Game.cpp](../src/Application/Game.cpp) returns a targeted scroll if the player cancels before choosing a target. The queued-cast cancellation path now restores an MM7 cursor scroll before dropping that spell, which also covers a caster becoming unable to act. A completed cast consumes one scroll.

The [GameTests](../test/Bin/GameTest/GameTests_MM7Scrolls.cpp) check Fire Aura, Fire Bolt and Cure Weakness cancellation and completion, plus an actor-target scroll read from the open inventory. `IncapacitationCancelsActorTargetScroll` proves that an unconscious caster closes the target picker and returns Fire Bolt to the cursor without damaging the target. The five scroll cases passed three consecutive headless runs on 2026-11-11.

Other targeting exits, including attacking, passing the turn, stealing, a full spell queue and turn-based wait, remain in the audit queue. No visible game run has checked the installed Atlas since the latest headless installation.

See also: [Campaign Status](Campaign-Status.md), [MM7 Disposable Save and Reload](MM7-Disposable-Save-Reload.md), [Agent Handoff](Agent-Handoff.md).
