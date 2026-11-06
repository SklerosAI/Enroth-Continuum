---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, media, portraits]
---

# MM6 Media F2 Closure Source

The [media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) and tracked [consumer probe](../scripts/mm6_probe_media_consumers.py) compare inherited portrait, creation-icon, font and reaction-sound names with retained MM6 indexes. The inherited 22 portrait first-frame names and constructed runtime frame names have no match in the retained four-archive icon index. Only part of the inherited class-icon requests match. All literal font loads are indexed, while the older font layout and two fonts without literal load calls still need classification. Possible inherited voice reaction IDs have many table gaps. These are name/index and source-formula findings, not evidence that original MM6 lacks portraits, that another archive cannot supply them, or that pictures or samples fail in the running game.

Current VID/SND readers use inherited directory record types, movie lookup appends inherited suffixes, house descriptors use MM7 mappings, and spell/voice sound IDs are numeric inheritances. The consolidated F3 preflight must confirm original directory and face resources before F4 title-specific mapping. F5 must inspect frames and hear playback, including vanilla resources before any private portrait override.

The [media trigger map](../project/MM6_MEDIA_TRIGGER_MAP_F2.md) joins all 67 map music references and eight active event-sound records to retained names/IDs, while 116 distinct MM6 room IDs yield no inherited MM7 movie-name matches. Six current movie literals include three raw retained base names, but suffix-form lookup matches none of the extensionless retained VID names. Inherited spell sound IDs also collide with present MM6 IDs, which is coverage rather than cue correctness. Current opcode-12 movie dispatch conflicts with the MM6 texture-operation packet; F3 must identify original movie triggers and house sound/movie assignments. The 15 soundtrack tracks are part of the prepared original size/hash identity check.

Authority: [project/MM6_MEDIA_F2_CLOSURE.md](../project/MM6_MEDIA_F2_CLOSURE.md), [trigger map](../project/MM6_MEDIA_TRIGGER_MAP_F2.md), [media/UI specification](../project/MM6_MEDIA_UI_SPEC.md) and [implementation checklist](../project/MM6_IMPLEMENTATION_CHECKLIST.md). Exact resource names, potential sound IDs and absence lists stay ignored locally; no MM6 media or complete UI has been verified.

See also: [MM6 Media and UI Specification Source](Source-MM6-Media-UI-Spec.md), [MM6 Source and Probe Index](MM6-Source-and-Probe-Index.md), [Open Questions](Open-Questions.md).
