---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, media, ui]
---

# MM6 Media and UI Specification Source

This F2 source compares retained media indexes with MM6's current video, house, sound, music, and character UI consumers.

The retained video index presents base names while the inherited movie lookup adds extensions; F3 must check the raw directory before choosing a lookup policy. Inherited MM7 house video descriptors do not match the retained MM6 names. The MM6 sound table has repeated IDs and names without an indexed archive match, requiring an explicit alias and fallback policy. Map music files are present, while playback remains unobserved. Character creation still uses MM7 class and portrait assumptions. These are source-routing gaps, not a verified rendering or audio test.

Authority: [project/MM6_MEDIA_UI_SPEC.md](../project/MM6_MEDIA_UI_SPEC.md), reviewed 2026-11-05. Its current C++ consumers and F3/F5 checks remain the reference; this wiki does not reproduce a media name inventory.

See also: [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md).
