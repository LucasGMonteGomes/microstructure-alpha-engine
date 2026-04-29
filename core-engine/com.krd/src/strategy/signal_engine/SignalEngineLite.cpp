#include "SignalEngineLite.h"

#include <cmath>
#include <sstream>

SignalEngineLite::SignalEngineLite(const StrategyConfig& config)
    : config_(config) {}

SignalResult SignalEngineLite::evaluate(const MarketSnapshot& snapshot,
                                        const FlowSnapshot& flowSnapshot,
                                        double recentMoveBps) const {
    SignalResult result;

    result.flowBias = flowSnapshot.aggressionBias;
    result.flowStrength = flowSnapshot.totalAggressionQty;

    // ------------------------------------------------------------------
    // Filtro de spread
    // ------------------------------------------------------------------

    if (snapshot.spreadBps > config_.maxSpreadBps) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "spread too high";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Filtro de movimento recente
    // ------------------------------------------------------------------

    if (std::abs(recentMoveBps) > config_.maxRecentMoveBps) {
        result.side = SignalSide::HOLD;
        result.confidence = 0.0;
        result.expectedMoveBps = 0.0;
        result.reason = "recent move too large";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Cálculo de qualidade do sinal
    // ------------------------------------------------------------------

    const double confidence = calculateConfidence(snapshot, flowSnapshot, recentMoveBps);
    const double expectedMoveBps = calculateExpectedMoveBps(snapshot, flowSnapshot, recentMoveBps);

    result.confidence = confidence;
    result.expectedMoveBps = expectedMoveBps;

    // ------------------------------------------------------------------
    // Condições estruturais
    // ------------------------------------------------------------------

    const bool bookLong  = snapshot.imbalance >= config_.imbalanceLongThreshold;
    const bool bookShort = snapshot.imbalance <= config_.imbalanceShortThreshold;

    const bool flowStrongEnough = flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
    const bool flowLong  = flowSnapshot.aggressionBias >= config_.minFlowBiasAbs;
    const bool flowShort = flowSnapshot.aggressionBias <= -config_.minFlowBiasAbs;

    const bool confidenceOk    = confidence >= config_.minConfidenceSignalLite;
    const bool expectedMoveOk  = expectedMoveBps >= config_.minExpectedMoveBps;

    // ------------------------------------------------------------------
    // Sinal LONG
    // ------------------------------------------------------------------

    if (bookLong && flowStrongEnough && flowLong && confidenceOk && expectedMoveOk) {
        result.side = SignalSide::LONG;
        result.reason = "long signal: imbalance + strong aggressive buy flow + controlled spread + low recent move";
        result.isValid = true;
        return result;
    }

    // ------------------------------------------------------------------
    // Sinal SHORT
    // ------------------------------------------------------------------

    if (bookShort && flowStrongEnough && flowShort && confidenceOk && expectedMoveOk) {
        result.side = SignalSide::SHORT;
        result.reason = "short signal: negative imbalance + strong aggressive sell flow + controlled spread + low recent move";
        result.isValid = true;
        return result;
    }

    // ------------------------------------------------------------------
    // Diagnóstico de rejeição
    // ------------------------------------------------------------------

    std::ostringstream reason;

    if (!flowStrongEnough) {
        reason << "flow too weak;";
    }
    if (!confidenceOk) {
        reason << " confidence below min;";
    }
    if (!expectedMoveOk) {
        reason << " expected move below min;";
    }
    if (bookLong && !flowLong && flowStrongEnough) {
        reason << " book long but flow not confirming long;";
    }
    if (bookShort && !flowShort && flowStrongEnough) {
        reason << " book short but flow not confirming short;";
    }
    if (bookLong && flowShort) {
        reason << " book-flow conflict: long book vs short flow;";
    }
    if (bookShort && flowLong) {
        reason << " book-flow conflict: short book vs long flow;";
    }
    if (!bookLong && !bookShort) {
        reason << " imbalance not extreme enough;";
    }

    std::string finalReason = reason.str();
    if (finalReason.empty()) {
        finalReason = "no valid setup";
    }

    result.side = SignalSide::HOLD;
    result.reason = finalReason;
    result.isValid = false;
    return result;
}

// =============================================================================
// Cálculo de confiança
// =============================================================================

double SignalEngineLite::calculateConfidence(const MarketSnapshot& snapshot,
                                             const FlowSnapshot& flowSnapshot,
                                             double recentMoveBps) const {
    const double imbalanceStrength =
        std::abs(snapshot.imbalance - 50.0) / 50.0;

    double flowStrength = std::abs(flowSnapshot.aggressionBias);
    if (flowStrength > 1.0) flowStrength = 1.0;

    double spreadPenalty = snapshot.spreadBps / config_.maxSpreadBps;
    if (spreadPenalty > 1.0) spreadPenalty = 1.0;

    double movePenalty = std::abs(recentMoveBps) / config_.maxRecentMoveBps;
    if (movePenalty > 1.0) movePenalty = 1.0;

    const double combinedStrength =
        (imbalanceStrength * config_.slConfidenceWeightImbalance) +
        (flowStrength      * config_.slConfidenceWeightFlow);

    double confidence = combinedStrength
                      * (1.0 - config_.slSpreadPenaltyFactor * spreadPenalty)
                      * (1.0 - config_.slMovePenaltyFactor   * movePenalty);

    if (confidence < 0.0) confidence = 0.0;
    if (confidence > 1.0) confidence = 1.0;

    return confidence;
}

// =============================================================================
// Cálculo de movimento esperado
// =============================================================================

double SignalEngineLite::calculateExpectedMoveBps(const MarketSnapshot& snapshot,
                                                  const FlowSnapshot& flowSnapshot,
                                                  double recentMoveBps) const {
    const double imbalanceStrength = std::abs(snapshot.imbalance - 50.0);
    const double flowStrength =
        std::abs(flowSnapshot.aggressionBias) * config_.slExpectedMoveFlowFactor;

    double expectedMove =
        (imbalanceStrength * config_.slExpectedMoveImbalanceWeight) +
        (flowStrength      * config_.slExpectedMoveFlowWeight);

    expectedMove -= std::abs(recentMoveBps) * config_.slExpectedMovePenaltyFactor;

    if (expectedMove < 0.0) expectedMove = 0.0;

    return expectedMove;
}
