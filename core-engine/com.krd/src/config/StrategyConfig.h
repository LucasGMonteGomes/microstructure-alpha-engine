#pragma once

// =============================================================================
// StrategyConfig — Parâmetros específicos de todas as estratégias
//
// Separado do Config (infraestrutura) para que cada estratégia tenha seus
// parâmetros de calibração centralizados sem poluir a config global.
//
// Estratégias cobertas:
//   - BreakoutConfirmationV2Strategy
//   - RegimeHoldContinuationStrategy
//   - SignalEngineLite
// =============================================================================

struct StrategyConfig {

    // =========================================================================
    // Filtros compartilhados entre estratégias
    // =========================================================================

    // Spread máximo permitido em basis points para aceitar entrada.
    double maxSpreadBps{6.0};

    // Movimento recente máximo em basis points.
    // Movimentos acima desse valor indicam mercado já em movimento — sem entrada.
    double maxRecentMoveBps{80.0};

    // Movimento esperado mínimo em basis points para justificar entrada após custos.
    double minExpectedMoveBps{25.0};

    // Imbalance mínimo (%) para considerar o book favorável a compra.
    double imbalanceLongThreshold{58.0};

    // Imbalance máximo (%) para considerar o book favorável a venda.
    double imbalanceShortThreshold{42.0};

    // Viés mínimo absoluto do fluxo agressor para confirmar direção.
    double minFlowBiasAbs{0.68};

    // Volume mínimo de agressão recente para considerar o fluxo ativo.
    double minFlowStrength{0.02};

    // =========================================================================
    // BreakoutConfirmationV2Strategy
    // =========================================================================

    // Faixa mínima em basis points para que o breakout seja relevante.
    double minRangeBps{0.20};

    // Faixa máxima em basis points para considerar o mercado comprimido.
    double maxRangeBps{8.0};

    // Confiança mínima para permitir entrada na estratégia de breakout.
    double minConfidenceBreakout{0.45};

    // Divide totalAggressionQty por esse valor para normalizar em [0, 1].
    double flowStrengthNormFactor{5.0};

    // Divide (shortRangeBps + activityBps) / 2 por esse valor para normalizar.
    double regimeStrengthNormFactor{4.0};

    // Divide breakoutDistanceBps por esse valor para normalizar em [0, 1].
    double breakoutDistanceNormFactor{12.0};

    // Pesos da confiança (breakout) — devem somar 1.0
    double confidenceWeightFlowBias{0.22};
    double confidenceWeightFlowStrength{0.18};
    double confidenceWeightImbalance{0.15};
    double confidenceWeightRegime{0.20};
    double confidenceWeightBreakoutDistance{0.25};

    // Fatores do expectedMoveBps (breakout)
    double expectedMoveFlowBiasFactor{12.0};
    double expectedMoveFlowStrengthFactor{1.2};
    double flowStrengthCapForMove{6.0};
    double expectedMoveImbalanceFactor{0.12};
    double expectedMoveRegimeFactor{4.0};
    double expectedMoveBreakoutFactor{3.5};

    // =========================================================================
    // RegimeHoldContinuationStrategy
    // =========================================================================

    // Range mínimo do regime para considerar o mercado ativo o suficiente.
    double regimeMinShortRangeBps{1.20};

    // Atividade mínima do regime para considerar o mercado ativo o suficiente.
    double regimeMinActivityBps{1.20};

    // Movimento mínimo em bps para confirmar direção (long ou short).
    double minRecentMoveBps{0.50};

    // Confiança mínima para entrada na estratégia de regime hold.
    double minConfidenceRegimeHold{0.70};

    // Divide totalAggressionQty por esse valor para normalizar (regime hold).
    double rhFlowStrengthNormFactor{5.0};

    // Divide (shortRangeBps + activityBps) / 2 por esse valor para normalizar (regime hold).
    double rhRegimeStrengthNormFactor{5.0};

    // Divide recentMoveBps por esse valor para normalizar (regime hold).
    double rhRecentMoveNormFactor{5.0};

    // Pesos da confiança (regime hold) — devem somar 1.0
    double rhConfidenceWeightFlowBias{0.20};
    double rhConfidenceWeightFlowStrength{0.15};
    double rhConfidenceWeightImbalance{0.15};
    double rhConfidenceWeightRegime{0.30};
    double rhConfidenceWeightRecentMove{0.20};

    // Fatores do expectedMoveBps (regime hold)
    double rhExpectedMoveFlowBiasFactor{12.0};
    double rhExpectedMoveFlowStrengthFactor{1.2};
    double rhFlowStrengthCapForMove{6.0};
    double rhExpectedMoveImbalanceFactor{0.18};
    double rhExpectedMoveRegimeFactor{6.0};
    double rhExpectedMoveMoveFactor{3.5};

    // =========================================================================
    // SignalEngineLite
    // =========================================================================

    // Confiança mínima para permitir entrada no SignalEngineLite.
    double minConfidenceSignalLite{0.45};

    // Pesos da confiança (signal engine lite)
    double slConfidenceWeightImbalance{0.55};
    double slConfidenceWeightFlow{0.45};

    // Fatores de penalidade aplicados à confiança
    double slSpreadPenaltyFactor{0.5};
    double slMovePenaltyFactor{0.5};

    // Fatores do expectedMoveBps (signal engine lite)
    double slExpectedMoveImbalanceFactor{0.5};
    double slExpectedMoveFlowFactor{20.0};
    double slExpectedMoveFlowWeight{0.8};
    double slExpectedMoveImbalanceWeight{0.5};
    double slExpectedMovePenaltyFactor{0.5};
};
