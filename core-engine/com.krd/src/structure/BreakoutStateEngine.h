#pragma once

#include <string>
#include <unordered_map>

#include "../domain/BreakoutState.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/PriceStructureSnapshot.h"

class BreakoutStateEngine {
public:
    BreakoutState update(const MarketSnapshot& snapshot,
                         const PriceStructureSnapshot& structure);

    void markEntryConsumed(const std::string& exchange, const std::string& symbol);

private:
    std::unordered_map<std::string, BreakoutState> stateByKey_;

    std::string makeKey(const std::string& exchange, const std::string& symbol) const;
};