#pragma once

#include "../../config/Config.h"
#include "../common/ISignalStrategy.h"

class BreakoutConfirmationV2Strategy : public ISignalStrategy {
public:
    explicit BreakoutConfirmationV2Strategy(const Config& config);

    SignalResult evaluate(const StrategyContext& context) const override;

private:
    const Config& config_;

    bool isSpreadAcceptable(const StrategyContext& context) const;
    bool isRegimeSupportive(const StrategyContext& context) const;

    bool isBreakoutUpValid(const StrategyContext& context) const;
    bool isBreakoutDownValid(const StrategyContext& context) const;

    bool isFlowConfirmingUp(const StrategyContext& context) const;
    bool isFlowConfirmingDown(const StrategyContext& context) const;

    bool isBookSupportiveUp(const StrategyContext& context) const;
    bool isBookSupportiveDown(const StrategyContext& context) const;

    bool isBreakoutStateUsableUp(const StrategyContext& context) const;
    bool isBreakoutStateUsableDown(const StrategyContext& context) const;

    double calculateConfidence(const StrategyContext& context) const;
    double calculateExpectedMoveBps(const StrategyContext& context) const;
};