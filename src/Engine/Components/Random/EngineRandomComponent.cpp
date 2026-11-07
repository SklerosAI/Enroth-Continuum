#include "EngineRandomComponent.h"

#include <cassert>
#include <memory>
#include <stdexcept>

#include "Engine/Random/Random.h"

#include "Library/Platform/Application/PlatformApplication.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"
#include "Library/Random/SequentialRandomEngine.h"

#include "TracingRandomEngine.h"

static EngineRandomComponent *globalRandomComponent = nullptr;

static std::unique_ptr<RandomEngine> createRandomEngine(RandomEngineType type) {
    if (type == RANDOM_ENGINE_MERSENNE_TWISTER) {
        return std::make_unique<MersenneTwisterRandomEngine>();
    } else {
        assert(type == RANDOM_ENGINE_SEQUENTIAL);
        return std::make_unique<SequentialRandomEngine>();
    }
}

EngineRandomComponent::EngineRandomComponent() {
    assert(globalRandomComponent == nullptr);
    globalRandomComponent = this;
}

EngineRandomComponent::~EngineRandomComponent() {
    assert(globalRandomComponent == this);
    globalRandomComponent = nullptr;
}

bool EngineRandomComponent::isTracing() const {
    return _tracing;
}

void EngineRandomComponent::setTracing(bool tracing) {
    if (_tracing == tracing)
        return;

    _tracing = tracing;
    swizzleGlobals();
}

RandomEngineType EngineRandomComponent::type() const {
    return _type;
}

void EngineRandomComponent::setType(RandomEngineType type) {
    if (_type == type)
        return;

    _type = type;
    swizzleGlobals();
}

void EngineRandomComponent::seed(int seed) {
    for (RandomEngineType type : _grngs.indices()) {
        _vrngs[type]->seed(seed);
        _grngs[type]->seed(seed);
    }
}

GameplayRandomState EngineRandomComponent::captureGameplayState() const {
    if (_type != RANDOM_ENGINE_MERSENNE_TWISTER && _type != RANDOM_ENGINE_SEQUENTIAL)
        throw std::runtime_error("unsupported gameplay RNG type");
    if (!_grngs[_type])
        throw std::runtime_error("gameplay RNG is not installed");
    GameplayRandomState result;
    result.type = _type;
    if (_type == RANDOM_ENGINE_MERSENNE_TWISTER) {
        result.mtState = static_cast<MersenneTwisterRandomEngine *>(_grngs[_type].get())->stateText();
    } else if (_type == RANDOM_ENGINE_SEQUENTIAL) {
        result.sequentialState = static_cast<SequentialRandomEngine *>(_grngs[_type].get())->state();
    } else {
        throw std::runtime_error("unsupported gameplay RNG type");
    }
    return result;
}

void EngineRandomComponent::restoreGameplayState(const GameplayRandomState &state) {
    if (_type != RANDOM_ENGINE_MERSENNE_TWISTER && _type != RANDOM_ENGINE_SEQUENTIAL)
        throw std::runtime_error("unsupported gameplay RNG type");
    if (!_grngs[_type] || state.type != _type)
        throw std::runtime_error("gameplay RNG type differs from saved state");
    if (_type == RANDOM_ENGINE_MERSENNE_TWISTER) {
        if (state.sequentialState)
            throw std::runtime_error("Mersenne Twister state has sequential data");
        static_cast<MersenneTwisterRandomEngine *>(_grngs[_type].get())->restoreStateText(state.mtState);
    } else if (_type == RANDOM_ENGINE_SEQUENTIAL) {
        if (!state.mtState.empty())
            throw std::runtime_error("sequential RNG state has Mersenne Twister data");
        static_cast<SequentialRandomEngine *>(_grngs[_type].get())->restoreState(state.sequentialState);
    } else {
        throw std::runtime_error("unsupported gameplay RNG type");
    }
}

void EngineRandomComponent::installNotify() {
    for (RandomEngineType type : _grngs.indices()) {
        _vrngs[type] = createRandomEngine(type);
        _grngs[type] = createRandomEngine(type);
        _tracingGrngs[type] = std::make_unique<TracingRandomEngine>(application()->platform(), _grngs[type].get());
    }
    swizzleGlobals();
}

void EngineRandomComponent::removeNotify() {
    for (RandomEngineType type : _grngs.indices()) {
        _vrngs[type].reset();
        _grngs[type].reset();
        _tracingGrngs[type].reset();
    }
    swizzleGlobals(); // Set globals to nullptr.
}

void EngineRandomComponent::swizzleGlobals() {
    vrng = _vrngs[_type].get();
    grng = (_tracing ? _tracingGrngs : _grngs)[_type].get();
}
