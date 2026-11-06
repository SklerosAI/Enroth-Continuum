---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, media, ui]
---

# MM6 Media and UI Specification Source

This F2 source compares retained media indexes with MM6's current video, house, sound, music, and character UI consumers.

The retained video index presents base names while the inherited movie lookup adds extensions; F3 must check the raw directory before choosing a lookup policy. Inherited MM7 house video descriptors do not match the retained MM6 names. The MM6 sound table has repeated IDs and names without an indexed archive match, requiring an explicit alias and fallback policy. Map music files are present, while playback remains unobserved. Character creation still uses MM7 class and portrait assumptions. These are source-routing gaps, not a verified rendering or audio test.

A pinned [MMExtension executable-table packet](../project/MM6_EXECUTABLE_TABLE_LOCATORS_F2.md) identifies a candidate 119-entry MM6 house-movie structure with fields for file, background, NPC picture, type and sound group. All 116 distinct retained room IDs fit its stated bounds. The owner's actual table values and pointer targets remain unverified until F3; the MM7 descriptor array is still the current C++ consumer.

The [media-schema packet](../project/MM6_MEDIA_SCHEMA_LEADS_F2.md) adds pinned primary-source 44-byte VID and 52-byte SND directory shapes, compatible in width with current readers. The retained video names are extensionless while current lookup adds a suffix; the final VID payload/trailer and raw owner bytes still require F3. Twelve retained 01–53 icon-frame groups with uniform 59×79 extracted PNG headers are strong portrait candidates, not identified or rendered faces.

The tracked [house-route probe](../scripts/mm6_probe_house_routes.py) joins active `SpeakInHouse` instructions to retained house rows and the parser's literal type vocabulary. It finds 674 active entries aimed at 521 distinct house IDs: 519 have a retained row, and two are special terminal handlers. The legacy parser sent 384 active nonempty labels to `HOUSE_TYPE_MERCENARY_GUILD`; the [F4 parser](MM6-F4-House-Type-Routing.md) now classifies all 557 retained rows with no active unknown label. Some room animation IDs are shared across type labels, and shop/guild helpers still use MM7 numeric ID ranges. These are static routing findings, not proof of intended original services, route reachability, or an observed crash. Per-event details stay in the ignored report.

The [media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) adds lexical checks of inherited creation and runtime portrait names, icons, font loads and generated reaction sound IDs. The inherited portrait names are absent from the retained four-archive icon index; class icons match only partly. This does not establish the original portrait archive or any rendered failure. Literal font names resolve in the retained index, but a present file is not proof of correctly rendered text.

Authority: [project/MM6_MEDIA_UI_SPEC.md](../project/MM6_MEDIA_UI_SPEC.md), reviewed 2026-11-05. Its current C++ consumers and F3/F5 checks remain the reference; this wiki does not reproduce a media name inventory.

See also: [MM6 Media F2 Closure Source](Source-MM6-Media-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md).
