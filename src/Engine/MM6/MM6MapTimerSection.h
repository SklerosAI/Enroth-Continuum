#pragma once

#include "Engine/GameTitle.h"

#include "MM6MapVisit.h"
#include "MM6SaveContainer.h"

namespace mm6 {

/**
 * @param firings                       Last periodic timer firings of every map, see `mapTimerFirings`.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Optional `TIMR` section. Throws for a map name or a time the section can't hold.
 */
SaveSection encodeMapTimerSection(const MapTimerFirings &firings, GameTitle title);

/**
 * @param container                     Container with at most one `TIMR` section.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Decoded firings, empty for a save written before the section existed. Throws
 *                                      on a malformed section.
 */
MapTimerFirings decodeMapTimerSection(const SaveContainer &container, GameTitle title);

} // namespace mm6
