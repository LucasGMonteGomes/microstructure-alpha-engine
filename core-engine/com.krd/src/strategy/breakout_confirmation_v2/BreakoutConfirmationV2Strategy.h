#pragma once

#include "com.krd/src/config/StrategyConfig.h"
#include "com.krd/src/domain/SignalResult.h"
#include "com.krd/src/strategy/common/StrategyContext.h"

// =============================================================================
// BreakoutConfirmationV2Strategy
//
// Responsabilidade: avaliar o contexto de mercado e gerar sinal de entrada
// baseado em breakout com confirmação de fluxo, book e regime.
//
// Condições para sinal LONG:
//   - Regime tradable
//   - Spread aceitável
//   - Breakout UP válido (CONFIRMED_UP, faixa dentro do range, não consumido)
//   - Fluxo confirmando compra
//   - Book com imbalance favorável a compra
//   - Confiança acima do mínimo
//   - ExpectedMove acima do mínimo
//
// Condições para sinal SHORT: simétricas para baixo.
// =============================================================================

class BreakoutConfirmationV2Strategy {
public:
    explicit BreakoutConfirmationV2Strategy(const StrategyConfig& config);

    SignalResult evaluate(const StrategyContext& context) const;

private:
    const StrategyConfig& config_;

    bool isSpreadAcceptable(const StrategyContext& context) const;
    bool isRegimeSupportive(const StrategyContext& context) const;

    bool isBreakoutUpValid(const StrategyContext& context) const;
    bool isBreakoutDownValid(const StrategyContext& context) const;

    bool isFlowConfirmingUp(const StrategyContext& context) const;
    bool isFlowConfirmingDown(const StrategyContext& context) const;

    bool isBookSupportiveUp(const StrategyContext& context) const;
    bool isBookSupportiveDown(const StrategyContext& context) const;

    double calculateConfidence(const StrategyContext& context) const;
    double calculateExpectedMoveBps(const StrategyContext& context) const;
};
