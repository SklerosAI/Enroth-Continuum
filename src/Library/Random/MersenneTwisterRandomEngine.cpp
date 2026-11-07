#include "MersenneTwisterRandomEngine.h"

#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

std::string MersenneTwisterRandomEngine::stateText() const {
    std::ostringstream output;
    output.imbue(std::locale::classic());
    output << _base;
    return output.str();
}

void MersenneTwisterRandomEngine::restoreStateText(std::string_view state) {
    if (state.empty() || state.size() > 8192)
        throw std::runtime_error("invalid Mersenne Twister state length");
    std::istringstream input{std::string(state)};
    input.imbue(std::locale::classic());
    std::mt19937 next;
    if (!(input >> next))
        throw std::runtime_error("invalid Mersenne Twister state");
    input >> std::ws;
    if (!input.eof())
        throw std::runtime_error("trailing Mersenne Twister state text");
    _base = next;
}
