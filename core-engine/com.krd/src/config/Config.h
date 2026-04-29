#pragma once

#include <cstdint>

// =============================================================================
// Config — Infraestrutura, execução, filtros e janelas de tempo
//
// Responsabilidade: parâmetros que controlam o funcionamento do motor,
// filtros de execução, janelas de dados e simulação de custos.
// Parâmetros específicos da estratégia estão em StrategyConfig.h
// =============================================================================

struct Config {

    // -------------------------------------------------------------------------
    // Duração do teste
    // -------------------------------------------------------------------------

    // Duração total do teste em milissegundos.
    // Exemplos:
    //   5  * 60 * 1000 =  5 minutos
    //   15 * 60 * 1000 = 15 minutos
    //   60 * 60 * 1000 =  1 hora
    long testDurationMs{15L * 60L * 1000L};

    // -------------------------------------------------------------------------
    // Janelas de dados
    // -------------------------------------------------------------------------

    // Janela de tempo usada pelo AggressionTracker para acumular trades.
    std::int64_t aggressionWindowMs{5000};

    // Janela de tempo usada pelo RegimeFilter para avaliar o regime de mercado.
    std::int64_t regimeWindowMs{3000};

    // Janela de tempo usada pelo PriceStructureEngine para manter histórico de preços.
    std::int64_t priceStructureWindowMs{30000};

    // -------------------------------------------------------------------------
    // BreakoutStateEngine
    // -------------------------------------------------------------------------

    // Tempo máximo (ms) que um breakout pode permanecer no estado returnedInsideRange
    // antes de ser marcado como FAILED. Se o preço voltou para dentro da faixa e
    // ficou lá por mais do que esse tempo, o breakout é encerrado.
    std::int64_t breakoutFailedTimeoutMs{5000};

    // -------------------------------------------------------------------------
    // Filtro de persistência do sinal
    // -------------------------------------------------------------------------

    // Número mínimo de snapshots consecutivos com sinal válido antes de permitir entrada.
    int persistenceMinCount{0};

    // Número máximo de snapshots que o filtro aguarda antes de resetar.
    int persistenceMaxCount{1};

    // -------------------------------------------------------------------------
    // Filtro de spread
    // -------------------------------------------------------------------------

    // Spread máximo permitido em basis points para aceitar entrada.
    double maxSpreadBps{6.0};

    // -------------------------------------------------------------------------
    // Filtro de execução
    // -------------------------------------------------------------------------

    // Movimento esperado mínimo em basis points para justificar entrada após custos.
    double minExpectedMoveBps{25.0};

    // -------------------------------------------------------------------------
    // PaperTradeEngine — simulação de execução
    // -------------------------------------------------------------------------

    // Alvo de saída em porcentagem acima/abaixo do preço de entrada.
    double targetPct{0.70};

    // Stop de saída em porcentagem abaixo/acima do preço de entrada.
    double stopPct{0.25};

    // Timeout máximo da operação em milissegundos.
    // Após esse tempo sem atingir alvo ou stop, a posição é encerrada.
    long timeoutMs{15000};

    // -------------------------------------------------------------------------
    // Custos de simulação
    // -------------------------------------------------------------------------

    // Taxa cobrada por lado da operação (entrada ou saída), em porcentagem.
    double feePctPerSide{0.01};

    // Slippage estimado por lado da operação, em porcentagem.
    double slippagePctPerSide{0.0025};
};
