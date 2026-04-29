#pragma once

#include "../../config/StrategyConfig.h"
#include "../common/ISignalStrategy.h"

// =============================================================================
// RegimeHoldContinuationStrategy
//
// Responsabilidade: gerar sinal quando o regime de mercado está ativo e o
// fluxo, book e movimento recente estão todos alinhados na mesma direção.
//
// Condições para sinal LONG:
//   - Regime tradable e forte (shortRangeBps + activityBps acima do mínimo)
//   - Spread aceitável
//   - Fluxo agressor confirmando compra
//   - Book com imbalance favorável a compra
//   - Movimento recente positivo dentro do limite
//   - Confiança acima do mínimo
//
// Condições para sinal SHORT: simétricas para baixo.
// =============================================================================

class RegimeHoldContinuationStrategy : public ISignalStrategy {
public:
    explicit RegimeHoldContinuationStrategy(const StrategyConfig& config);

    SignalResult evaluate(const StrategyContext& context) const override;

private:
    const StrategyConfig& config_;

    bool isSpreadAcceptable(const StrategyContext& context) const;
    bool isRegimeStrongEnough(const StrategyContext& context) const;

    bool isLongFlowAligned(const StrategyContext& context) const;
    bool isShortFlowAligned(const StrategyContext& context) const;

    bool isLongBookAligned(const StrategyContext& context) const;
    bool isShortBookAligned(const StrategyContext& context) const;

    bool isLongMoveConfirmed(const StrategyContext& context) const;
    bool isShortMoveConfirmed(const StrategyContext& context) const;

    double calculateConfidence(const StrategyContext& context) const;
    double calculateExpectedMoveBps(const StrategyContext& context) const;
};
