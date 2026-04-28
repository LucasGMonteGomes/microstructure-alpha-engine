#pragma once

#include <deque>
#include <string>
#include <unordered_map>

#include "../config/Config.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/PriceStructureSnapshot.h"

class PriceStructureEngine {
public:
    explicit PriceStructureEngine(const Config& config);

    PriceStructureSnapshot update(const MarketSnapshot& snapshot);

private:
    struct PricePoint {
        double price{0.0};
        std::int64_t timestampMs{0};
    };

    const Config& config_;

    std::unordered_map<std::string, std::deque<PricePoint>> historyByKey_;

    std::string makeKey(const std::string& exchange, const std::string& symbol) const;

    void trimHistory(std::deque<PricePoint>& history, std::int64_t nowMs) const;

    double calculateRangeBps(double lowPrice, double highPrice) const;
    double calculateBreakoutDistanceBps(double referencePrice, double currentPrice) const;
};