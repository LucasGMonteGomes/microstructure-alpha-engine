#include "RegimeHoldContinuationStrategy.h"

#include <algorithm>
#include <cmath>

RegimeHoldContinuationStrategy::RegimeHoldContinuationStrategy(const StrategyConfig& config)
    : config_(config) {}

SignalResult RegimeHoldContinuationStrategy::evaluate(const StrategyContext& context) const {
    SignalResult result;

    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    result.flowBias = flow.aggressionBias;
    result.flowStrength = flow.totalAggressionQty;

    // ------------------------------------------------------------------
    // Filtros obrigatórios — rejeição imediata
    // ------------------------------------------------------------------

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

    if (!isRegimeStrongEnough(context)) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "regime too weak for regime hold continuation";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Avaliação de confirmações por direção
    // ------------------------------------------------------------------

    const bool longFlow  = isLongFlowAligned(context);
    const bool shortFlow = isShortFlowAligned(context);

    const bool longBook  = isLongBookAligned(context);
    const bool shortBook = isShortBookAligned(context);

    const bool longMove  = isLongMoveConfirmed(context);
    const bool shortMove = isShortMoveConfirmed(context);

    // ------------------------------------------------------------------
    // Cálculo de qualidade do sinal
    // ------------------------------------------------------------------

    result.confidence = calculateConfidence(context);
    result.expectedMoveBps = calculateExpectedMoveBps(context);

    // ------------------------------------------------------------------
    // Sem fluxo alinhado
    // ------------------------------------------------------------------

    if (!longFlow && !shortFlow) {
        result.side = SignalSide::HOLD;
        result.reason = "flow not aligned for regime hold continuation";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Confirmação incompleta para LONG
    // ------------------------------------------------------------------

    if (longFlow && (!longBook || !longMove)) {
        result.side = SignalSide::HOLD;
        result.reason = "long regime hold continuation not aligned";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Confirmação incompleta para SHORT
    // ------------------------------------------------------------------

    if (shortFlow && (!shortBook || !shortMove)) {
        result.side = SignalSide::HOLD;
        result.reason = "short regime hold continuation not aligned";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Filtros de qualidade mínima
    // ------------------------------------------------------------------

    if (result.confidence < config_.minConfidenceRegimeHold) {
        result.side = SignalSide::HOLD;
        result.reason = "regime hold continuation confidence below min";
        result.isValid = false;
        return result;
    }

    if (result.expectedMoveBps < config_.minExpectedMoveBps) {
        result.side = SignalSide::HOLD;
        result.reason = "regime hold continuation expected move below min";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Sinal LONG
    // ------------------------------------------------------------------

    if (longFlow && longBook && longMove) {
        result.side = SignalSide::LONG;
        result.reason = "long regime hold continuation detected";
        result.isValid = true;
        return result;
    }

    // ------------------------------------------------------------------
    // Sinal SHORT
    // ------------------------------------------------------------------

    if (shortFlow && shortBook && shortMove) {
        result.side = SignalSide::SHORT;
        result.reason = "short regime hold continuation detected";
        result.isValid = true;
        return result;
    }

    result.side = SignalSide::HOLD;
    result.reason = "regime hold continuation unresolved";
    result.isValid = false;
    return result;
}

// =============================================================================
// Filtros
// =============================================================================

bool RegimeHoldContinuationStrategy::isSpreadAcceptable(const StrategyContext& context) const {
    return context.marketSnapshot.spreadBps <= config_.maxSpreadBps;
}

bool RegimeHoldContinuationStrategy::isRegimeStrongEnough(const StrategyContext& context) const {
    return context.regimeSnapshot.shortRangeBps >= config_.regimeMinShortRangeBps &&
           context.regimeSnapshot.activityBps >= config_.regimeMinActivityBps &&
           context.regimeSnapshot.hasRecentFlow;
}

bool RegimeHoldContinuationStrategy::isLongFlowAligned(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias >= config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool RegimeHoldContinuationStrategy::isShortFlowAligned(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias <= -config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool RegimeHoldContinuationStrategy::isLongBookAligned(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance >= config_.imbalanceLongThreshold;
}

bool RegimeHoldContinuationStrategy::isShortBookAligned(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance <= config_.imbalanceShortThreshold;
}

bool RegimeHoldContinuationStrategy::isLongMoveConfirmed(const StrategyContext& context) const {
    return context.recentMoveBps >= config_.minRecentMoveBps &&
           context.recentMoveBps <= config_.maxRecentMoveBps;
}

bool RegimeHoldContinuationStrategy::isShortMoveConfirmed(const StrategyContext& context) const {
    return context.recentMoveBps <= -config_.minRecentMoveBps &&
           std::abs(context.recentMoveBps) <= config_.maxRecentMoveBps;
}

// =============================================================================
// Cálculo de confiança
// =============================================================================

double RegimeHoldContinuationStrategy::calculateConfidence(
    const StrategyContext& context) const {

    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    const double flowBiasStrength = std::clamp(
        std::abs(flow.aggressionBias), 0.0, 1.0);

    const double flowStrengthNorm = std::clamp(
        flow.totalAggressionQty / config_.rhFlowStrengthNormFactor, 0.0, 1.0);

    const double imbalanceStrength = std::clamp(
        std::abs(snapshot.imbalance - 50.0) / 50.0, 0.0, 1.0);

    const double regimeRaw = (regime.shortRangeBps + regime.activityBps) / 2.0;
    const double regimeStrengthNorm = std::clamp(
        regimeRaw / config_.rhRegimeStrengthNormFactor, 0.0, 1.0);

    const double recentMoveStrength = std::clamp(
        std::abs(context.recentMoveBps) / config_.rhRecentMoveNormFactor, 0.0, 1.0);

    const double confidence =
        (flowBiasStrength    * config_.rhConfidenceWeightFlowBias) +
        (flowStrengthNorm    * config_.rhConfidenceWeightFlowStrength) +
        (imbalanceStrength   * config_.rhConfidenceWeightImbalance) +
        (regimeStrengthNorm  * config_.rhConfidenceWeightRegime) +
        (recentMoveStrength  * config_.rhConfidenceWeightRecentMove);

    return std::clamp(confidence, 0.0, 1.0);
}

// =============================================================================
// Cálculo de movimento esperado
// =============================================================================

double RegimeHoldContinuationStrategy::calculateExpectedMoveBps(
    const StrategyContext& context) const {

    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;

    const double flowBiasComponent =
        std::abs(flow.aggressionBias) * config_.rhExpectedMoveFlowBiasFactor;

    const double flowStrengthComponent =
        std::min(flow.totalAggressionQty, config_.rhFlowStrengthCapForMove) *
        config_.rhExpectedMoveFlowStrengthFactor;

    const double imbalanceComponent =
        std::abs(snapshot.imbalance - 50.0) * config_.rhExpectedMoveImbalanceFactor;

    const double regimeComponent =
        (regime.shortRangeBps + regime.activityBps) * config_.rhExpectedMoveRegimeFactor;

    const double moveComponent =
        std::abs(context.recentMoveBps) * config_.rhExpectedMoveMoveFactor;

    const double expectedMove =
        flowBiasComponent +
        flowStrengthComponent +
        imbalanceComponent +
        regimeComponent +
        moveComponent;

    return std::max(expectedMove, 0.0);
}
