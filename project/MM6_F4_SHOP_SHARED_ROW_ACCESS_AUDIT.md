# MM6 F4 shared shop and guild row access audit

Status, 2026-11-06: the accepted F3-matched original executable gives a **bounded direct-reference map** for the shared stock row. It does not yet give a complete proof of every indirect writer or a nonzero native saved example. The detached `SHOP` section and refresh helper remain outside live services and the required known-save manifest.

The last active shop's second bank and the first active spell guild address one physical twelve-record row and one refresh deadline. A read-only survey of the retained executable's stock-base references found these source families:

| Source family | What was established |
| --- | --- |
| New-game initializer | Clears the full 28-byte physical records, including the shared row. |
| Shop and guild generators | Both shop banks and the guild can write the shared row according to which service refreshes an expired deadline. The shop generators pass record pointers to a general item generator, so direct-address stores alone do not describe every item-field write. |
| House menu and purchase | Menu branches read item IDs from the row after refresh arbitration. Successful purchase copies the complete 28-byte item to inventory and clears the source's first 25 bytes. |
| Listing, hover and pricing | Additional paths construct row-record pointers and read item IDs or item details. The inspected direct base operands in those paths are reads or pointer construction; effects of every downstream call have not been ruled out. |
| Original Party persistence | Raw Party serialization includes the stock area. It does not define a field-level conversion into the fork's runtime `Item`. |

This is a direct-anchor survey, **not an exhaustive dataflow proof**. Indirect aliases, calls receiving stock pointers and bulk Party operations still require call-side tracing or a controlled live session. No owner-local standalone native save with nonzero shared stock was found in the staged MM6 Game Data or ignored local game data. Installation archives do not establish the stock of a played and saved shared edge.

The [item conversion audit](MM6_F4_SHOP_ITEM_RECORD_CONVERSION_GATE.md) details the remaining field gap. The original shop generator can clear an entire 28-byte record and call a general item generator through a computed pointer. The fork's inherited `Item_MM7` snapshot is 36 bytes and carries fields whose MM6 meaning has not been proved. The bounded MM6 chest projection checks only the first 24 bytes and retains the last four as an opaque sidecar; that check does not authorize shop generation, purchase or save restoration. A complete MM6 item mapping and live MM6 stock owner are needed before admitting `SHOP` to the required save transaction.

Next, trace every downstream call that receives a computed stock pointer and validate a nonzero native save from an owner-local played session. Then establish a title-local service owner with a full-record item conversion and a late-failure nonmutation check. Keep the current MM7 and normal MM6 SaveLoad guards.

See [shared-edge refresh](MM6_F4_SHOP_SHARED_EDGE_REFRESH.md), [purchase write](MM6_F4_SHOP_PURCHASE_WRITE_GATE.md), and [shop owner field gate](MM6_F4_SHOP_OWNER_FIELD_GATE.md). Exact offsets and private checks remain ignored locally.
