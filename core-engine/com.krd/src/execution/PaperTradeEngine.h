#pragma once

#include <optional>

#include "../config/Config.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/Position.h"
#include "../domain/SignalResult.h"
#include "../domain/TradeResult.h"

// =============================================================================
// PaperTradeEngine
//
// Responsabilidade: simular abertura, monitoramento e fechamento de posições
// com base em sinais da estratégia e snapshots de mercado.
//
// Saídas possíveis:
//   TAKE_PROFIT  — preço atingiu o alvo configurado
//   STOP_LOSS    — preço atingiu o stop configurado
//   INERTIA      — trade não se moveu o suficiente após earlyExitAfterMs
//   TIMEOUT      — tempo máximo da operação atingido
// =============================================================================

class PaperTradeEngine {
public:
    explicit PaperTradeEngine(const Config& config);

    bool hasOpenPosition() const;
    const Position& getOpenPosition() const;

    // Tenta abrir posição. Retorna false se já há posição aberta ou sinal inválido.
    bool tryOpenPosition(const MarketSnapshot& snapshot, const SignalResult& signal);

    // Monitora posição aberta. Retorna TradeResult se a posição foi fechada.
    std::optional<TradeResult> update(const MarketSnapshot& snapshot,
                                      const SignalResult& latestSignal);

    // Força fechamento da posição aberta com o motivo informado.
    std::optional<TradeResult> forceClosePosition(const MarketSnapshot& snapshot,
                                                  ExitReason reason);

private:
    const Config& config_;
    Position currentPosition_;

    double calculateGrossPnlPct(const Position& position, double exitPrice) const;
    double calculateNetPnlPct(double grossPnlPct) const;

    double buildTargetPrice(const MarketSnapshot& snapshot, SignalSide side) const;
    double buildStopPrice(const MarketSnapshot& snapshot, SignalSide side) const;

    // Verifica se o trade está inerte: passou earlyExitAfterMs sem mover o suficiente.
    bool shouldExitByInertia(const MarketSnapshot& snapshot) const;

    TradeResult closePosition(const MarketSnapshot& snapshot, ExitReason reason);
};
