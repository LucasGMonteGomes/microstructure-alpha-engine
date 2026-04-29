#pragma once

#include <optional>

#include "../config/Config.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/Position.h"
#include "../domain/SignalResult.h"
#include "../domain/TradeResult.h"

class PaperTradeEngine {
public:
    explicit PaperTradeEngine(const Config& config);

    bool hasOpenPosition() const;
    const Position& getOpenPosition() const;

    bool tryOpenPosition(const MarketSnapshot& snapshot, const SignalResult& signal);

    std::optional<TradeResult> update(const MarketSnapshot& snapshot,
                                      const SignalResult& latestSignal);

    std::optional<TradeResult> forceClosePosition(const MarketSnapshot& snapshot,
                                                  ExitReason reason);

private:
    const Config& config_;
    Position currentPosition_;

    double calculateGrossPnlPct(const Position& position, double exitPrice) const;
    double calculateNetPnlPct(double grossPnlPct) const;

    double buildTargetPrice(const MarketSnapshot& snapshot, SignalSide side) const;
    double buildStopPrice(const MarketSnapshot& snapshot, SignalSide side) const;

    bool shouldExitEarly(const SignalResult& latestSignal) const;

    TradeResult closePosition(const MarketSnapshot& snapshot, ExitReason reason);
};