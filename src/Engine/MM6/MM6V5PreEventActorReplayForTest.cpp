#include "MM6V5PreEventActorReplayForTest.h"

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "MM6ActiveActorSection.h"
#include "MM6V5OutdoorMapIdentity.h"

namespace mm6::testing {

ScopedV5PreEventActorReplayForTest::ScopedV5PreEventActorReplayForTest(
    GameTitle title, ActorRuntimeSaveFields fields,
    std::vector<std::uint8_t> baselineActr)
    : _fields(std::move(fields)), _baselineActr(std::move(baselineActr)) {
    if (title != GameTitle::MM6 || detail::activeV5ActorReplayScope() ||
        !isMM6OutdoorMapName(_fields.mapName) || _fields.actors.empty())
        throw std::runtime_error("MM6 v5 actor replay needs one staged outdoor owner");
    requireActorRuntimeBaselineAgreement(
        _fields, _fields.mapName, _baselineActr, title);
    detail::activeV5ActorReplayScope() = this;
}

ScopedV5PreEventActorReplayForTest::~ScopedV5PreEventActorReplayForTest() {
    if (detail::activeV5ActorReplayScope() == this)
        detail::activeV5ActorReplayScope() = nullptr;
}

bool runArmedV5PreEventActorReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::deque<Actor> &baseline,
    std::deque<Actor> *destination,
    bool v3Published,
    std::string_view loadedMapName,
    GameTitle title) {
    ScopedV5PreEventActorReplayForTest *scope = detail::activeV5ActorReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !v3Published || !destination || destination != &baseline ||
        loadedMapName != scope->_fields.mapName ||
        baseline.size() != scope->_fields.actors.size())
        throw std::runtime_error("MM6 v5 actor replay lacks an exact v4 baseline context");
    const std::vector<Actor> baselineCopy(baseline.begin(), baseline.end());
    if (encodeActiveActorSection(baselineCopy, title) != scope->_baselineActr)
        throw std::runtime_error("MM6 v5 actor replay baseline changed before ARTR");
    std::deque<Actor> dynamic(scope->_fields.actors.begin(), scope->_fields.actors.end());
    static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
    destination->swap(dynamic);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
