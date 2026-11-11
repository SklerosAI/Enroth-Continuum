#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "Core/Time/Duration.h"
#include "Core/Time/Time.h"

#include "Engine/Evt/EvtEnums.h"

namespace mm6 {

/**
 * @param deltaBytes                    Decoded `.ddm` or `.dlv` delta. Its last 56 bytes are the visit block: the last
 *                                      visit time, then the sky name shown on that visit and the fog fields.
 * @return                              When the party last left the map, zero for a map it never left.
 */
Time mapLastVisit(std::span<const std::uint8_t> deltaBytes);

/**
 * @param deltaBytes                    Decoded `.ddm` delta.
 * @return                              Sky bitmap name stored in the visit block.
 */
std::string mapVisitSky(std::span<const std::uint8_t> deltaBytes);

/**
 * Writes the visit block the way MM6 does each time it saves the map's delta, when the party leaves the map and when
 * the game is saved. Its other fields keep their bytes.
 *
 * @param[in,out] deltaBytes            Decoded `.ddm` or `.dlv` delta.
 * @param leaveTime                     Party's playing time.
 * @param sky                           Sky bitmap shown on this visit, empty for an indoor map, which keeps its bytes.
 */
void recordMapVisit(std::span<std::uint8_t> deltaBytes, Time leaveTime, std::string_view sky);

/**
 * First alarm an MM6 map load gives an `OnTimer` or `OnLongTimer` trigger that has no half-minute interval.
 *
 * @param trigger                       `EVENT_OnTimer` or `EVENT_OnLongTimer`.
 * @param interval                      One year, 28 days, 7 days or one day.
 * @param timeInsideDay                 Time of day a daily timer goes off at.
 * @param lastVisit                     When the party last left the map, zero for a map it never left.
 * @param now                           Party's playing time.
 * @return                              Zero for an `OnLongTimer` on a first visit or a day or more after the last one.
 *                                      Otherwise the same time one interval later, or for a daily timer the time of day
 *                                      tomorrow.
 */
Time mapTimerFirstAlarm(EvtOpcode trigger, Duration interval, Duration timeInsideDay, Time lastVisit, Time now);

} // namespace mm6
