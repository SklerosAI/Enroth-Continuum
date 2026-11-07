#pragma once

#include <memory>
#include <cstdint>
#include <string>

#include "Engine/Random/RandomEnums.h"

#include "Library/Platform/Application/PlatformApplicationAware.h"

#include "Utility/IndexedArray.h"

class RandomEngine;
class Platform;

struct GameplayRandomState {
    RandomEngineType type = RANDOM_ENGINE_MERSENNE_TWISTER;
    std::string mtState;
    std::uint32_t sequentialState = 0;

    bool operator==(const GameplayRandomState &) const = default;
};

class EngineRandomComponent : public PlatformApplicationAware {
 public:
    EngineRandomComponent();
    ~EngineRandomComponent();

    [[nodiscard]] bool isTracing() const;
    void setTracing(bool tracing);

    [[nodiscard]] RandomEngineType type() const;
    void setType(RandomEngineType type);

    /**
     * Seeds all random engines.
     *
     * @param seed                      Seed to pass to `RandomEngine::seed`.
     */
    void seed(int seed);

    // Captures/restores only the selected gameplay endpoint. Tracing wraps
    // that same underlying engine, so it does not change the saved state.
    GameplayRandomState captureGameplayState() const;
    void restoreGameplayState(const GameplayRandomState &state);

 private:
    virtual void installNotify() override;
    virtual void removeNotify() override;
    void swizzleGlobals();

    using RandomEnginesArray = IndexedArray<std::unique_ptr<RandomEngine>, RANDOM_ENGINE_FIRST, RANDOM_ENGINE_LAST>;

 private:
    RandomEnginesArray _grngs;
    RandomEnginesArray _vrngs;
    RandomEnginesArray _tracingGrngs;
    bool _tracing = false;
    RandomEngineType _type = RANDOM_ENGINE_MERSENNE_TWISTER;
};
