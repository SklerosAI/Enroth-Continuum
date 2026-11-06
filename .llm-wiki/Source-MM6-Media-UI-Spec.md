---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, media, ui]
---

# MM6 Media and UI Specification Source

This F2 source compares retained media indexes with MM6's current video, house, sound, music, and character UI consumers.

The retained video index presents base names while the inherited movie lookup adds extensions; F3 must check the raw directory before choosing a lookup policy. Inherited MM7 house video descriptors do not match the retained MM6 names. The MM6 sound table has repeated IDs and names without an indexed archive match, requiring an explicit alias and fallback policy. Map music files are present, while playback remains unobserved. Character creation still uses MM7 class and portrait assumptions. These are source-routing gaps, not a verified rendering or audio test.

The tracked [house-route probe](../scripts/mm6_probe_house_routes.py) joins active `SpeakInHouse` instructions to retained house rows and the parser's literal type vocabulary. It finds 674 active entries aimed at 521 distinct house IDs: 519 have a retained row, and two are special terminal handlers. Of the table-backed targets, 384 use nonempty labels the current parser silently maps to `HOUSE_TYPE_MERCENARY_GUILD`; `createHouseUI` then chooses the corresponding inherited UI. Some room animation IDs are shared across type labels, and shop/guild helpers use MM7 numeric ID ranges. These are static routing findings, not proof of intended original services, route reachability, or an observed crash. Per-event details stay in the ignored report.

Authority: [project/MM6_MEDIA_UI_SPEC.md](../project/MM6_MEDIA_UI_SPEC.md), reviewed 2026-11-05. Its current C++ consumers and F3/F5 checks remain the reference; this wiki does not reproduce a media name inventory.

See also: [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md).
