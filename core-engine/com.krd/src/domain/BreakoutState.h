#pragma once

#include <cstdint>
#include <string>

#include "BreakoutPhase.h"

struct BreakoutState {
    std::string exchange;
    std::string symbol;

    BreakoutPhase phase{BreakoutPhase::IDLE};

    double breakPrice{0.0};
    double lastConfirmedPrice{0.0};

    double highestPriceAfterBreak{0.0};
    double lowestPriceAfterBreak{0.0};

    bool returnedInsideRange{false};
    bool active{false};
    bool entryConsumed{false};

    std::int64_t breakTimestampMs{0};
};
