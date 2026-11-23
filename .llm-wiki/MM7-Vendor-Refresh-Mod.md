---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[Runtime-Isolation]]", "[[MM6-F4-Shop-Purchase-Write-Gate]]"]
tags: [mm7, mod, vendor, shop]
---

# MM7 Vendor Refresh Mod

The optional MM7 vendor mod regenerates stock once when a new vendor house window opens.

The [implementation packet](../project/MM7_VENDOR_REFRESH_MOD.md) records the exact entry boundary and opt-in setting. The Atlas supplies `ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY=1` only for a modded MM7 launch. The same Atlas toggle also sets `ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY` for MM6 launches (`launcher/GameBuild.swift`), which the MM6 helper reads in `src/Engine/MM6/MM6ShopRefreshBoundary.cpp`. Shops regenerate both tiers. Magic guilds regenerate books. The normal next-refresh deadline prevents another reroll while navigating buying submenus in the same visit. The default interval applies when the flag is absent.

The MM7 title and one-visit guard passed two linked synthetic tests. The focused MM6/MM7 vendor set passed 16/16 and non-MM6 unit controls passed 440/440. A mod-enabled MM7 headless save/load smoke passed 1/1. Two headless game tests entered a real MM7 weapon shop and magic guild, confirmed stock generation on entry, and confirmed Buy-tab navigation did not regenerate again (2/2, no visible window or focus change). The installed Atlas carries the toggle. A visible play-session shop check is open.

See also: [Campaign Status](Campaign-Status.md), [Runtime Isolation](Runtime-Isolation.md), [MM7 Private Portrait Mod](MM7-Private-Portrait-Mod.md), [MM6 Shop Purchase Gate](MM6-F4-Shop-Purchase-Write-Gate.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
