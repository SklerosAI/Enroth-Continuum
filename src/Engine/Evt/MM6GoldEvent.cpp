#include "Engine/Evt/MM6GoldEvent.h"

#include <limits>
#include <stdexcept>

namespace mm6 {
namespace {

void checkNonnegative(int balance, int amount) {
    if (balance < 0 || amount < 0)
        throw std::runtime_error("MM6 gold event has negative balance or operand");
}

} // namespace

bool hasEventGold(int balance, int amount) {
    checkNonnegative(balance, amount);
    return balance >= amount;
}

int goldAfterEventAdd(int balance, int amount) {
    checkNonnegative(balance, amount);
    if (amount > std::numeric_limits<int>::max() - balance)
        throw std::runtime_error("MM6 gold event addition exceeds stored balance range");
    return balance + amount;
}

std::optional<int> goldAfterEventSubtract(int balance, int amount) {
    checkNonnegative(balance, amount);
    if (amount > balance)
        return std::nullopt;
    return balance - amount;
}

} // namespace mm6
