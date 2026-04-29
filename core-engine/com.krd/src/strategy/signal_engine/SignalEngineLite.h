#pragma once

#include "../../config/StrategyConfig.h"
#include "../../domain/MarketSnapshot.h"
#include "../../domain/SignalResult.h"
#include "../../domain/FlowSnapshot.h"

// =============================================================================
// SignalEngineLite
//
// Responsabilidade: avaliar snapshot + fluxo + movimento recente e gerar
// sinal básico de entrada. Não usa regime nem estrutura de preço — é o
// motor mais simples do sistema, focado em imbalance e fluxo imediato.
// =============================================================================

class SignalEngineLite {
public:
    explicit SignalEngineLite(const StrategyConfig& config);

    SignalResult evaluate(const MarketSnapshot& snapshot,
                          const FlowSnapshot& flowSnapshot,
                          double recentMoveBps) const;

private:
    const StrategyConfig& config_;

    double calculateConfidence(const MarketSnapshot& snapshot,
                               const FlowSnapshot& flowSnapshot,
                               double recentMoveBps) const;

    double calculateExpectedMoveBps(const MarketSnapshot& snapshot,
                                    const FlowSnapshot& flowSnapshot,
                                    double recentMoveBps) const;
};
