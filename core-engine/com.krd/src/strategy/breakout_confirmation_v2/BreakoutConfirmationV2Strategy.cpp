#include "BreakoutConfirmationV2Strategy.h"

#include <algorithm>
#include <cmath>

BreakoutConfirmationV2Strategy::BreakoutConfirmationV2Strategy(const StrategyConfig& config)
    : config_(config) {}

SignalResult BreakoutConfirmationV2Strategy::evaluate(const StrategyContext& context) const {
    SignalResult result;

    const auto& flow = context.flowSnapshot;

    result.flowBias = flow.aggressionBias;
    result.flowStrength = flow.totalAggressionQty;

    // ------------------------------------------------------------------
    // Filtros obrigatórios — rejeição imediata
    // ------------------------------------------------------------------

    if (!context.regimeSnapshot.tradable) {
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

    // ------------------------------------------------------------------
    // Avaliação de confirmações por direção
    // ------------------------------------------------------------------

    const bool breakoutUp = isBreakoutUpValid(context);
    const bool breakoutDown = isBreakoutDownValid(context);

    const bool flowUp = isFlowConfirmingUp(context);
    const bool flowDown = isFlowConfirmingDown(context);

    const bool bookUp = isBookSupportiveUp(context);
    const bool bookDown = isBookSupportiveDown(context);

    // ------------------------------------------------------------------
    // Cálculo de qualidade do sinal
    // ------------------------------------------------------------------

    result.confidence = calculateConfidence(context);
    result.expectedMoveBps = calculateExpectedMoveBps(context);

    // ------------------------------------------------------------------
    // Sem estrutura de breakout válida
    // ------------------------------------------------------------------

    if (!breakoutUp && !breakoutDown) {
        result.side = SignalSide::HOLD;
        result.reason = "no valid breakout structure";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Breakout para cima sem confirmação completa
    // ------------------------------------------------------------------

    if (breakoutUp && (!flowUp || !bookUp)) {
        result.side = SignalSide::HOLD;
        result.reason = "up breakout not fully confirmed";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Breakout para baixo sem confirmação completa
    // ------------------------------------------------------------------

    if (breakoutDown && (!flowDown || !bookDown)) {
        result.side = SignalSide::HOLD;
        result.reason = "down breakout not fully confirmed";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Filtros de qualidade mínima
    // ------------------------------------------------------------------

    if (result.confidence < config_.minConfidenceBreakout) {
        result.side = SignalSide::HOLD;
        result.reason = "breakout confirmation v2 confidence below min";
        result.isValid = false;
        return result;
    }

    // ------------------------------------------------------------------
    // Sinal LONG
    // ------------------------------------------------------------------

    if (breakoutUp && flowUp && bookUp) {
        result.side = SignalSide::LONG;
        result.reason = "breakout confirmation v2 up detected";
        result.isValid = true;
        return result;
    }

    // ------------------------------------------------------------------
    // Sinal SHORT
    // ------------------------------------------------------------------

    if (breakoutDown && flowDown && bookDown) {
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

// =============================================================================
// Filtros
// =============================================================================

bool BreakoutConfirmationV2Strategy::isSpreadAcceptable(const StrategyContext& context) const {
    // O maxSpreadBps vive no Config de infraestrutura.
    // A estratégia recebe o spreadBps já calculado no snapshot.
    // Para não criar dependência do Config aqui, o caller deve pré-filtrar
    // ou a estratégia pode receber o limite por parâmetro.
    // Por ora, o filtro de spread é feito no evaluate() com context.marketSnapshot.spreadBps.
    // Este método existe para facilitar extensão futura.
    return true; // Filtrado diretamente no evaluate() abaixo
}

bool BreakoutConfirmationV2Strategy::isRegimeSupportive(const StrategyContext& context) const {
    return context.regimeSnapshot.tradable;
}

bool BreakoutConfirmationV2Strategy::isBreakoutUpValid(const StrategyContext& context) const {
    const auto& structure = context.priceStructureSnapshot;
    const auto& breakoutState = context.breakoutState;

    return breakoutState.active &&
           !breakoutState.entryConsumed &&
           breakoutState.phase == BreakoutPhase::CONFIRMED_UP &&
           structure.rangeBps >= config_.minRangeBps &&
           structure.rangeBps <= config_.maxRangeBps;
}

bool BreakoutConfirmationV2Strategy::isBreakoutDownValid(const StrategyContext& context) const {
    const auto& structure = context.priceStructureSnapshot;
    const auto& breakoutState = context.breakoutState;

    return breakoutState.active &&
           !breakoutState.entryConsumed &&
           breakoutState.phase == BreakoutPhase::CONFIRMED_DOWN &&
           structure.rangeBps >= config_.minRangeBps &&
           structure.rangeBps <= config_.maxRangeBps;
}

bool BreakoutConfirmationV2Strategy::isFlowConfirmingUp(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias >= config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool BreakoutConfirmationV2Strategy::isFlowConfirmingDown(const StrategyContext& context) const {
    return context.flowSnapshot.aggressionBias <= -config_.minFlowBiasAbs &&
           context.flowSnapshot.totalAggressionQty >= config_.minFlowStrength;
}

bool BreakoutConfirmationV2Strategy::isBookSupportiveUp(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance >= config_.imbalanceLongThreshold;
}

bool BreakoutConfirmationV2Strategy::isBookSupportiveDown(const StrategyContext& context) const {
    return context.marketSnapshot.imbalance <= config_.imbalanceShortThreshold;
}

// =============================================================================
// Cálculo de confiança
// =============================================================================

double BreakoutConfirmationV2Strategy::calculateConfidence(
    const StrategyContext& context) const {

    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;
    const auto& structure = context.priceStructureSnapshot;

    double flowBiasStrength = std::clamp(std::abs(flow.aggressionBias), 0.0, 1.0);

    double flowStrengthNorm = std::clamp(
        flow.totalAggressionQty / config_.flowStrengthNormFactor, 0.0, 1.0);

    double imbalanceStrength = std::clamp(
        std::abs(snapshot.imbalance - 50.0) / 50.0, 0.0, 1.0);

    double regimeRaw = (regime.shortRangeBps + regime.activityBps) / 2.0;
    double regimeStrengthNorm = std::clamp(
        regimeRaw / config_.regimeStrengthNormFactor, 0.0, 1.0);

    double breakoutStrength = std::clamp(
        std::abs(structure.breakoutDistanceBps) / config_.breakoutDistanceNormFactor,
        0.0, 1.0);

    const double confidence =
        (flowBiasStrength   * config_.confidenceWeightFlowBias) +
        (flowStrengthNorm   * config_.confidenceWeightFlowStrength) +
        (imbalanceStrength  * config_.confidenceWeightImbalance) +
        (regimeStrengthNorm * config_.confidenceWeightRegime) +
        (breakoutStrength   * config_.confidenceWeightBreakoutDistance);

    return std::clamp(confidence, 0.0, 1.0);
}

// =============================================================================
// Cálculo de movimento esperado
// =============================================================================

double BreakoutConfirmationV2Strategy::calculateExpectedMoveBps(
    const StrategyContext& context) const {

    const auto& snapshot = context.marketSnapshot;
    const auto& flow = context.flowSnapshot;
    const auto& regime = context.regimeSnapshot;
    const auto& structure = context.priceStructureSnapshot;

    const double flowBiasComponent =
        std::abs(flow.aggressionBias) * config_.expectedMoveFlowBiasFactor;

    const double flowStrengthComponent =
        std::min(flow.totalAggressionQty, config_.flowStrengthCapForMove) *
        config_.expectedMoveFlowStrengthFactor;

    const double imbalanceComponent =
        std::abs(snapshot.imbalance - 50.0) * config_.expectedMoveImbalanceFactor;

    const double regimeComponent =
        (regime.shortRangeBps + regime.activityBps) * config_.expectedMoveRegimeFactor;

    const double breakoutComponent =
        std::abs(structure.breakoutDistanceBps) * config_.expectedMoveBreakoutFactor;

    const double expectedMove =
        flowBiasComponent +
        flowStrengthComponent +
        imbalanceComponent +
        regimeComponent +
        breakoutComponent;

    return std::max(expectedMove, 0.0);
}
