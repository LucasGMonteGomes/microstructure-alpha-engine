#pragma once

#include "../../config/Config.h"
#include "../common/ISignalStrategy.h"

class BreakoutConfirmationStrategy : public ISignalStrategy {
public:
    explicit BreakoutConfirmationStrategy(const Config& config);

    SignalResult evaluate(const StrategyContext& context) const override;

private:
    const Config& config_;

    bool isSpreadAcceptable(const StrategyContext& context) const;
    bool isRegimeSupportive(const StrategyContext& context) const;

    bool isLongBreakoutConfirmed(const StrategyContext& context) const;
    bool isShortBreakoutConfirmed(const StrategyContext& context) const;

    bool isLongBookSupportive(const StrategyContext& context) const;
    bool isShortBookSupportive(const StrategyContext& context) const;

    bool isLongMoveValid(const StrategyContext& context) const;
    bool isShortMoveValid(const StrategyContext& context) const;

    double calculateConfidence(const StrategyContext& context) const;
    double calculateExpectedMoveBps(const StrategyContext& context) const;
};