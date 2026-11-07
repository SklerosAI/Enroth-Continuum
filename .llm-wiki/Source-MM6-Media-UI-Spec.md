---
type: source-summary
up: "[[index_Enroth-Continuum]]"
tags: [source, mm6, media, ui]
---

# MM6 Media and UI Specification Source

This F2 source compares retained media indexes with MM6's current video, house, sound, music, and character UI consumers.

The retained video index presents base names while the inherited movie lookup adds extensions; F3 must check the raw directory before choosing a lookup policy. Inherited MM7 house video descriptors do not match the retained MM6 names. The MM6 sound table has repeated IDs and names without an indexed archive match, requiring an explicit alias and fallback policy. Map music files are present, while playback remains unobserved. Character creation still uses MM7 class and portrait assumptions. These are source-routing gaps, not a verified rendering or audio test.

A pinned [MMExtension executable-table packet](../project/MM6_EXECUTABLE_TABLE_LOCATORS_F2.md) identifies a candidate MM6 house-movie structure with fields for file, background, NPC picture, type and sound group. Retained room IDs fit its stated bounds. The owner's actual table values and pointer targets remain unverified until F3; the MM7 descriptor array is still the current C++ consumer.

The [media-schema packet](../project/MM6_MEDIA_SCHEMA_LEADS_F2.md) identified candidate 44-byte VID and 52-byte SND directory shapes. The [F3 media reconciliation](../project/MM6_F3_MEDIA_RECONCILIATION.md) verified owner VID entries with extensionless names and `SMK` payload headers; the [F4 movie lookup](MM6-F4-Movie-Lookup.md) now uses exact names for MM6. Decoded frames and audible playback remain unobserved. Retained portrait-frame candidates were subsequently reconciled to vanilla faces at F3; F5 still needs rendered portraits.

The tracked [house-route probe](../scripts/mm6_probe_house_routes.py) joins active `SpeakInHouse` instructions to retained house rows and the parser's literal type vocabulary. Active entries have retained rows or are special terminal handlers. The legacy parser misclassified active labels as `HOUSE_TYPE_MERCENARY_GUILD`; the [F4 parser](MM6-F4-House-Type-Routing.md) now classifies the retained rows with no active unknown label. Some room animation IDs are shared across type labels, and shop/guild helpers still use MM7 numeric ID ranges. These are static routing findings, not proof of intended original services, route reachability, or an observed crash. Per-event details stay in the ignored report.

The [media closure packet](../project/MM6_MEDIA_F2_CLOSURE.md) adds lexical checks of inherited creation and runtime portrait names, icons, font loads and generated reaction sound IDs. The inherited portrait names are absent from the retained four-archive icon index; class icons match only partly. This does not establish the original portrait archive or any rendered failure. Literal font names resolve in the retained index, but a present file is not proof of correctly rendered text.

Authority: [project/MM6_MEDIA_UI_SPEC.md](../project/MM6_MEDIA_UI_SPEC.md), reviewed 2026-11-05. Its current C++ consumers and F3/F5 checks remain the reference; this wiki does not reproduce a media name inventory.

See also: [MM6 Media F2 Closure Source](Source-MM6-Media-F2-Closure.md), [MM6 F2 Structural Findings](MM6-F2-Structural-Findings.md), [Open Questions](Open-Questions.md), [Campaign Status](Campaign-Status.md).
