#include "MM6GameplayRngSaveFields.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Library/Random/MersenneTwisterRandomEngine.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'R', 'N', 'G', 'S'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t MAX_MT_STATE = 8192;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 RNG section requires MM6 title");
}

void validate(const GameplayRandomState &state) {
    if (state.type == RANDOM_ENGINE_MERSENNE_TWISTER) {
        if (state.sequentialState || state.mtState.empty() ||
            state.mtState.size() > MAX_MT_STATE)
            throw std::runtime_error("invalid MM6 Mersenne Twister state shape");
        MersenneTwisterRandomEngine probe;
        probe.restoreStateText(state.mtState);
    } else if (state.type == RANDOM_ENGINE_SEQUENTIAL) {
        if (!state.mtState.empty())
            throw std::runtime_error("invalid MM6 sequential RNG state shape");
    } else {
        throw std::runtime_error("unsupported MM6 gameplay RNG type");
    }
}

void put16(std::vector<std::uint8_t> *out, std::uint16_t value) {
    out->push_back(value);
    out->push_back(value >> 8);
}

} // namespace

GameplayRandomState captureGameplayRngFields(
    const EngineRandomComponent &random, GameTitle title) {
    requireMM6(title);
    GameplayRandomState state = random.captureGameplayState();
    validate(state);
    return state;
}

SaveSection encodeGameplayRngSection(const GameplayRandomState &state,
                                     GameTitle title) {
    requireMM6(title);
    validate(state);
    std::vector<std::uint8_t> bytes{VERSION,
        static_cast<std::uint8_t>(state.type == RANDOM_ENGINE_SEQUENTIAL ? 1 : 0)};
    if (state.type == RANDOM_ENGINE_MERSENNE_TWISTER) {
        put16(&bytes, state.mtState.size());
        bytes.insert(bytes.end(), state.mtState.begin(), state.mtState.end());
    } else {
        for (int shift = 0; shift < 32; shift += 8)
            bytes.push_back(state.sequentialState >> shift);
    }
    return {TAG, true, std::move(bytes)};
}

GameplayRandomState decodeGameplayRngSection(const SaveContainer &container,
                                             GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 RNGS section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() < 4 ||
        section->payload.size() > MAX_MT_STATE + 4)
        throw std::runtime_error("missing or invalid MM6 RNGS section");
    const auto &bytes = section->payload;
    if (bytes[0] != VERSION || bytes[1] > 1)
        throw std::runtime_error("unsupported MM6 RNGS version or type");
    GameplayRandomState state;
    if (bytes[1] == 0) {
        const size_t length = std::uint16_t(bytes[2]) |
            (std::uint16_t(bytes[3]) << 8);
        if (length != bytes.size() - 4)
            throw std::runtime_error("MM6 RNGS Mersenne Twister length differs");
        state.mtState.assign(bytes.begin() + 4, bytes.end());
    } else {
        if (bytes.size() != 6)
            throw std::runtime_error("MM6 RNGS sequential length differs");
        state.type = RANDOM_ENGINE_SEQUENTIAL;
        for (int shift = 0; shift < 32; shift += 8)
            state.sequentialState |= std::uint32_t(bytes[2 + shift / 8]) << shift;
    }
    validate(state);
    return state;
}

} // namespace mm6
