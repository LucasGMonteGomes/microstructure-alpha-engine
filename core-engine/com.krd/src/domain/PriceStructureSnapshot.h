#pragma once

#include <cstdint>
#include <string>

struct PriceStructureSnapshot {
    std::string exchange;
    std::string symbol;

    double recentHigh{0.0};
    double recentLow{0.0};
    double midRangePrice{0.0};

    double rangeBps{0.0};
    double breakoutDistanceBps{0.0};

    bool isCompressed{false};
    bool isBreakoutUp{false};
    bool isBreakoutDown{false};

    std::int64_t timestampMs{0};
};