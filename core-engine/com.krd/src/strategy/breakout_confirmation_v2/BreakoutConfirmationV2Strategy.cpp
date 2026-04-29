#include "BreakoutConfirmationV2Strategy.h"

#include <algorithm>
#include <cmath>

BreakoutConfirmationV2Strategy::BreakoutConfirmationV2Strategy(const Config &config)
    : config_(config) {
}

SignalResult BreakoutConfirmationV2Strategy::evaluate(const StrategyContext &context) const {
    SignalResult result;

    const auto &snapshot = context.marketSnapshot;
    const auto &flow = context.flowSnapshot;
    const auto &regime = context.regimeSnapshot;
    const auto &structure = context.priceStructureSnapshot;
    const auto &breakoutState = context.breakoutState;

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
        result.reason = "regime not supportive for breakout confirmation v2";
        result.isValid = false;
        return result;
    }

    const bool breakoutUp = isBreakoutUpValid(context);
    const bool breakoutDown = isBreakoutDownValid(context);

    const bool flowUp = isFlowConfirmingUp(context);
    const bool flowDown = isFlowConfirmingDown(context);

    const bool bookUp = isBookSupportiveUp(context);
    const bool bookDown = isBookSupportiveDown(context);

    const bool stateUp = isBreakoutStateUsableUp(context);
    const bool stateDown = isBreakoutStateUsableDown(context);

    result.confidence = calculateConfidence(context);
    result.expectedMoveBps = calculateExpectedMoveBps(context);

    if (!breakoutUp && !breakoutDown) {
        result.side = SignalSide::HOLD;
        result.reason = "no valid breakout structure";
        result.isValid = false;
        return result;
    }

    if (breakoutUp && (!flowUp || !bookUp || !stateUp)) {
        result.side = SignalSide::HOLD;
        result.reason = "up breakout not fully confirmed";
        result.isValid = false;
        return result;
    }

    if (breakoutDown && (!flowDown || !bookDown || !stateDown)) {
        result.side = SignalSide::HOLD;
        result.reason = "down breakout not fully confirmed";
        result.isValid = false;
        return result;
    }

    if (result.confidence < config_.minConfidence) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout confirmation v2 confidence below min";
        result.isValid = false;
        return result;
    }

    if (result.expectedMoveBps < config_.minExpectedMoveBps) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout confirmation v2 expected move below min";
        result.isValid = false;
        return result;
    }

    if (breakoutUp && flowUp && bookUp && stateUp) {
        result.side = SignalSide::LONG;
        result.reason = "breakout confirmation v2 up detected";
        result.isValid = true;
        return result;
    }

    if (breakoutDown && flowDown && bookDown && stateDown) {
        result.side = SignalSide::SHORT;
        result.reason = "breakout confirmation v2 down detected";
        result.isValid = true;
        return result;
    }

    result.side = SignalSide::HOLD;
    result.reason = "breakout confirmation v2 unresolved";
    result.isValid = false;
    return result;
}

bool BreakoutConfirmationV2Strategy::isSpreadAcceptable(const StrategyContext &context) const {
    return context.marketSnapshot.spreadBps <= config_.maxSpreadBps;
}

bool BreakoutConfirmationV2Strategy::isRegimeSupportive(const StrategyContext &context) const {
    return context.regimeSnapshot.tradable;
}

bool BreakoutConfirmationV2Strategy::isBreakoutUpValid(const StrategyContext &context) const {
    const auto &structure = context.priceStructureSnapshot;
    const auto &breakoutState = context.breakoutState;

    const bool stateValid =
            breakoutState.active &&
            !breakoutState.entryConsumed &&
            breakoutState.phase == BreakoutPhase::CONFIRMED_UP;

    return stateValid &&
           structure.rangeBps >= 0.20 &&
           structure.rangeBps <= 8.0;
}

bool BreakoutConfirmationV2Strategy::isBreakoutDownValid(const StrategyContext &context) const {
    const auto &structure = context.priceStructureSnapshot;
    const auto &breakoutState = context.breakoutState;

    const bool stateValid =
            breakoutState.active &&
            !breakoutState.entryConsumed &&
            breakoutState.phase == BreakoutPhase::CONFIRMED_DOWN;

    return stateValid &&
           structure.rangeBps >= 0.20 &&
           structure.rangeBps <= 8.0;
}

bool BreakoutConfirmationV2Strategy::isFlowConfirmingUp(const StrategyContext &context) const {
    return context.flowSnapshot.aggressionBias >= config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool BreakoutConfirmationV2Strategy::isFlowConfirmingDown(const StrategyContext &context) const {
    return context.flowSnapshot.aggressionBias <= -config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool BreakoutConfirmationV2Strategy::isBookSupportiveUp(const StrategyContext &context) const {
    return context.marketSnapshot.imbalance >= config_.imbalanceLongThreshold;
}

bool BreakoutConfirmationV2Strategy::isBookSupportiveDown(const StrategyContext &context) const {
    return context.marketSnapshot.imbalance <= config_.imbalanceShortThreshold;
}

bool BreakoutConfirmationV2Strategy::isBreakoutStateUsableUp(const StrategyContext &context) const {
    return context.breakoutState.active &&
           context.breakoutState.phase != BreakoutPhase::FAILED &&
           (context.breakoutState.phase == BreakoutPhase::BREAK_UP ||
            context.breakoutState.phase == BreakoutPhase::HOLD_UP ||
            context.breakoutState.phase == BreakoutPhase::CONFIRMED_UP);
}

bool BreakoutConfirmationV2Strategy::isBreakoutStateUsableDown(const StrategyContext &context) const {
    return context.breakoutState.active &&
           context.breakoutState.phase != BreakoutPhase::FAILED &&
           (context.breakoutState.phase == BreakoutPhase::BREAK_DOWN ||
            context.breakoutState.phase == BreakoutPhase::HOLD_DOWN ||
            context.breakoutState.phase == BreakoutPhase::CONFIRMED_DOWN);
}

double BreakoutConfirmationV2Strategy::calculateConfidence(const StrategyContext &context) const {
    const auto &snapshot = context.marketSnapshot;
    const auto &flow = context.flowSnapshot;
    const auto &regime = context.regimeSnapshot;
    const auto &structure = context.priceStructureSnapshot;

    double flowBiasStrength = std::abs(flow.aggressionBias);
    flowBiasStrength = std::clamp(flowBiasStrength, 0.0, 1.0);

    double flowStrengthNorm = flow.totalAggressionQty / 5.0;
    flowStrengthNorm = std::clamp(flowStrengthNorm, 0.0, 1.0);

    double imbalanceStrength = std::abs(snapshot.imbalance - 50.0) / 50.0;
    imbalanceStrength = std::clamp(imbalanceStrength, 0.0, 1.0);

    double regimeStrength = (regime.shortRangeBps + regime.activityBps) / 2.0;
    double regimeStrengthNorm = std::clamp(regimeStrength / 4.0, 0.0, 1.0);

    double breakoutStrength = structure.breakoutDistanceBps / 12.0;
    breakoutStrength = std::clamp(std::abs(breakoutStrength), 0.0, 1.0);

    const double confidence =
            (flowBiasStrength * 0.22) +
            (flowStrengthNorm * 0.18) +
            (imbalanceStrength * 0.15) +
            (regimeStrengthNorm * 0.20) +
            (breakoutStrength * 0.25);

    return std::clamp(confidence, 0.0, 1.0);
}

double BreakoutConfirmationV2Strategy::calculateExpectedMoveBps(const StrategyContext &context) const {
    const auto &snapshot = context.marketSnapshot;
    const auto &flow = context.flowSnapshot;
    const auto &regime = context.regimeSnapshot;
    const auto &structure = context.priceStructureSnapshot;

    const double flowBiasComponent = std::abs(flow.aggressionBias) * 12.0;
    const double flowStrengthComponent = std::min(flow.totalAggressionQty, 6.0) * 1.2;
    const double imbalanceComponent = std::abs(snapshot.imbalance - 50.0) * 0.12;
    const double regimeComponent = (regime.shortRangeBps + regime.activityBps) * 4.0;
    const double breakoutComponent = std::abs(structure.breakoutDistanceBps) * 3.5;

    double expectedMove =
            flowBiasComponent +
            flowStrengthComponent +
            imbalanceComponent +
            regimeComponent +
            breakoutComponent;

    if (expectedMove < 0.0) {
        expectedMove = 0.0;
    }

    return expectedMove;
}
