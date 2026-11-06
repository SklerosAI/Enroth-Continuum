---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, save, persistence]
---

# MM6 Save Field Register F2 Source

The [F4 core field packet](../project/MM6_EC6_CORE_FIELDS_F4.md) now defines bounded `PART` and `CHAR` payloads and a copy-staging adapter for a limited party/character subset. The eleven-family whole-save register below remains the completeness gate; `.ec6` writing and live loading are still disabled.

The [whole-save field register](../project/MM6_SAVE_FIELD_REGISTER_F2.md) maps the current MM7 snapshot sections to logical state families the versioned MM6 `.ec6` save must persist: slot identity; party pose/time/resources; four characters; conditions, skills, spells, inventory and equipment; NPC, quest, award and note state; all 67 map deltas; overlays, beacons, thumbnail and active selection. Each family has an MM6-specific round-trip and visible acceptance scenario. It is not a native `.mm6` layout or a final `.ec6` field schema.

The current `loadGame()` changes some live state before MM7 deserialization succeeds, and `createSaveData()` updates in-memory map deltas before a direct destination write. F4 must validate into a temporary MM6 state and use a recoverable atomic save policy. F3 must reconcile title-specific field meanings and record unknowns in a private per-field manifest; F5 must verify saved state on screen after relaunch. The frozen MM7 serializer and profile remain separate.

Authority: [project/MM6_SAVE_FIELD_REGISTER_F2.md](../project/MM6_SAVE_FIELD_REGISTER_F2.md), reviewed 2026-11-05. See also: [MM6 Save Spec Source](Source-MM6-Save-Spec.md), [MM6 World F2 Closure Source](Source-MM6-World-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md).
