#include "BreakoutConfirmationStrategy.h"

#include <algorithm>
#include <cmath>

BreakoutConfirmationStrategy::BreakoutConfirmationStrategy(const Config& config)
    : config_(config) {}

SignalResult BreakoutConfirmationStrategy::evaluate(const StrategyContext& context) const {
    SignalResult result;

    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    result.flowBias = flow.aggressionBias;
    result.flowStrength = flow.totalAggressionQty;

    if (!regime.tradable) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "regime not tradable";
        result.isValid = false;
        return result;
    }

    if (!isSpreadAcceptable(context)) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "spread too high";
        result.isValid = false;
        return result;
    }

    if (!isRegimeSupportive(context)) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "regime not supportive for breakout confirmation";
        result.isValid = false;
        return result;
    }

    const bool longBreakout = isLongBreakoutConfirmed(context);
    const bool shortBreakout = isShortBreakoutConfirmed(context);

    const bool longBook = isLongBookSupportive(context);
    const bool shortBook = isShortBookSupportive(context);

    const bool longMove = isLongMoveValid(context);
    const bool shortMove = isShortMoveValid(context);

    result.confidence = calculateConfidence(context);
    result.expectedMoveBps = calculateExpectedMoveBps(context);

    if (!longBreakout && !shortBreakout) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout not confirmed";
        result.isValid = false;
        return result;
    }

    if (longBreakout && (!longBook || !longMove)) {
        result.side = SignalSide::HOLD;
        result.reason = "long breakout confirmation not aligned";
        result.isValid = false;
        return result;
    }

    if (shortBreakout && (!shortBook || !shortMove)) {
        result.side = SignalSide::HOLD;
        result.reason = "short breakout confirmation not aligned";
        result.isValid = false;
        return result;
    }

    if (result.confidence < 0.74) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout confirmation confidence below min";
        result.isValid = false;
        return result;
    }

    if (result.expectedMoveBps < 48.0) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout confirmation expected move below min";
        result.isValid = false;
        return result;
    }

    if (longBreakout && longBook && longMove) {
        result.side = SignalSide::LONG;
        result.reason = "long breakout confirmation detected";
        result.isValid = true;
        return result;
    }

    if (shortBreakout && shortBook && shortMove) {
        result.side = SignalSide::SHORT;
        result.reason = "short breakout confirmation detected";
        result.isValid = true;
        return result;
    }

    result.side = SignalSide::HOLD;
    result.reason = "breakout confirmation unresolved";
    result.isValid = false;
    return result;
}

bool BreakoutConfirmationStrategy::isSpreadAcceptable(const StrategyContext& context) const {
    return context.marketSnapshot.spreadBps <= config_.maxSpreadBps;
}

bool BreakoutConfirmationStrategy::isRegimeSupportive(const StrategyContext& context) const {
    return context.regimeSnapshot.shortRangeBps >= 1.0 &&
           context.regimeSnapshot.activityBps >= 1.0 &&
           context.regimeSnapshot.hasRecentFlow;
}

bool BreakoutConfirmationStrategy::isLongBreakoutConfirmed(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias >= 0.68 &&
           context.flowSnapshot.totalAggressionQty >= 1.05;
}

bool BreakoutConfirmationStrategy::isShortBreakoutConfirmed(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias <= -0.68 &&
           context.flowSnapshot.totalAggressionQty >= 1.05;
}

bool BreakoutConfirmationStrategy::isLongBookSupportive(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance >= 58.0;
}

bool BreakoutConfirmationStrategy::isShortBookSupportive(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance <= 42.0;
}

bool BreakoutConfirmationStrategy::isLongMoveValid(const StrategyContext& context) const {
    return context.recentMoveBps >= 1.35 &&
           context.recentMoveBps <= 10.0;
}

bool BreakoutConfirmationStrategy::isShortMoveValid(const StrategyContext& context) const {
    return context.recentMoveBps <= -1.35 &&
           std::abs(context.recentMoveBps) <= 10.0;
}

double BreakoutConfirmationStrategy::calculateConfidence(const StrategyContext& context) const {
    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    double flowBiasStrength = std::abs(flow.aggressionBias);
    flowBiasStrength = std::clamp(flowBiasStrength, 0.0, 1.0);

    double flowStrengthNorm = flow.totalAggressionQty / 5.0;
    flowStrengthNorm = std::clamp(flowStrengthNorm, 0.0, 1.0);

    double imbalanceStrength = std::abs(snapshot.imbalance - 50.0) / 50.0;
    imbalanceStrength = std::clamp(imbalanceStrength, 0.0, 1.0);

    double regimeStrength = (regime.shortRangeBps + regime.activityBps) / 2.0;
    double regimeStrengthNorm = std::clamp(regimeStrength / 4.0, 0.0, 1.0);

    double recentMoveStrength = std::abs(context.recentMoveBps) / 10.0;
    recentMoveStrength = std::clamp(recentMoveStrength, 0.0, 1.0);

    const double confidence =
        (flowBiasStrength * 0.22) +
        (flowStrengthNorm * 0.20) +
        (imbalanceStrength * 0.16) +
        (regimeStrengthNorm * 0.20) +
        (recentMoveStrength * 0.22);

    return std::clamp(confidence, 0.0, 1.0);
}

double BreakoutConfirmationStrategy::calculateExpectedMoveBps(const StrategyContext& context) const {
    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    const double flowBiasComponent = std::abs(flow.aggressionBias) * 12.0;
    const double flowStrengthComponent = std::min(flow.totalAggressionQty, 6.0) * 1.2;
    const double imbalanceComponent = std::abs(snapshot.imbalance - 50.0) * 0.15;
    const double regimeComponent = (regime.shortRangeBps + regime.activityBps) * 4.8;
    const double moveComponent = std::abs(context.recentMoveBps) * 3.0;

    double expectedMove =
        flowBiasComponent +
        flowStrengthComponent +
        imbalanceComponent +
        regimeComponent +
        moveComponent;

    if (expectedMove < 0.0) {
        expectedMove = 0.0;
    }

    return expectedMove;
}