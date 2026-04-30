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
    //int duration = 5  * 60 * 1000 // 5 minutos
    int duration = 15 * 60 * 1000; // 15 minutos
    //int duration 60 * 60 * 1000 // 1 hora
    long testDurationMs{duration};

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

    // Buffer mínimo de segurança em bps acima do custo total estimado
    // para permitir entrada. Usado pelo ExecutionQualityFilter.
    double executionSafetyBufferBps{8.0};

    // -------------------------------------------------------------------------
    // PaperTradeEngine — simulação de execução
    // -------------------------------------------------------------------------

    // Alvo de saída em porcentagem acima/abaixo do preço de entrada.
    double targetPct{0.03};

    // Stop de saída em porcentagem abaixo/acima do preço de entrada.
    double stopPct{0.025};

    // Timeout máximo da operação em milissegundos.
    // Após esse tempo sem atingir alvo ou stop, a posição é encerrada.
    long timeoutMs{20000};

    // -------------------------------------------------------------------------
    // Saída antecipada por inércia
    //
    // Se o trade não se moveu o suficiente após earlyExitAfterMs milissegundos,
    // considera que não tem direção e fecha para economizar custo de carregamento.
    // -------------------------------------------------------------------------

    // Tempo mínimo (ms) que o trade precisa ter aberto antes de avaliar inércia.
    // Deve ser menor que timeoutMs. Recomendado: metade do timeoutMs.
    long earlyExitAfterMs{10000};

    // Gross PnL mínimo (%) para considerar que o trade está se movendo na direção certa.
    // Se após earlyExitAfterMs o gross estiver abaixo desse valor, fecha antecipadamente.
    // Deve ser menor que targetPct. Recomendado: ~1/3 do targetPct.
    double earlyExitMinGrossPct{0.005};

    // -------------------------------------------------------------------------
    // Custos de simulação
    // -------------------------------------------------------------------------

    // Taxa cobrada por lado da operação (entrada ou saída), em porcentagem.
    double feePctPerSide{0.01};

    // Slippage estimado por lado da operação, em porcentagem.
    double slippagePctPerSide{0.0025};
};
