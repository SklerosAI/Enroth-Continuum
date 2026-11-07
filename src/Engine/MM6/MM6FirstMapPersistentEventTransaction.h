#pragma once

#include <stdexcept>
#include <type_traits>

#include "Engine/Engine.h"
#include "Engine/GameTitle.h"

namespace mm6 {

// Outdoor delta reconstruction, decoration respawn and onMapLoad events may
// each replace these same two fixed-size arrays during first-map entry. This
// scope protects only that owner; event trigger lists and event side effects
// elsewhere in Party/world/UI still need their own rollback policy.
class ScopedFirstMapPersistentEvents {
 public:
    ScopedFirstMapPersistentEvents(GameTitle title, PersistentVariables *variables)
        : _variables(variables) {
        if (title != GameTitle::MM6 || !variables)
            throw std::runtime_error("MM6 first-map persistent events require a title-local owner");
        static_assert(std::is_nothrow_copy_assignable_v<PersistentVariables>);
        _prior = *variables;
        _active = true;
    }

    ScopedFirstMapPersistentEvents(const ScopedFirstMapPersistentEvents &) = delete;
    ScopedFirstMapPersistentEvents &operator=(const ScopedFirstMapPersistentEvents &) = delete;
    ~ScopedFirstMapPersistentEvents() { rollback(); }

    void rollback() noexcept {
        if (!_active)
            return;
        *_variables = _prior;
        _active = false;
    }

    void commit() noexcept { _active = false; }

 private:
    PersistentVariables *_variables;
    PersistentVariables _prior = {};
    bool _active = false;
};

} // namespace mm6
