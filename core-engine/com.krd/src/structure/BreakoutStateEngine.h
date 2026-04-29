#pragma once

#include <string>
#include <unordered_map>

#include "../config/Config.h"
#include "../domain/BreakoutState.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/PriceStructureSnapshot.h"

// =============================================================================
// BreakoutStateEngine
//
// Responsabilidade: manter e evoluir o estado do breakout por ativo.
//
// O breakout não é um evento único de snapshot — ele tem fases que persistem
// entre snapshots: BREAK → HOLD → CONFIRMED ou FAILED.
//
// Transições de fase:
//   IDLE / FAILED  →  BREAK_UP / BREAK_DOWN   (novo breakout detectado)
//   BREAK_UP       →  CONFIRMED_UP            (breakout continua acima)
//   BREAK_UP       →  HOLD_UP                 (preço voltou para dentro)
//   HOLD_UP        →  CONFIRMED_UP            (breakout retomou)
//   HOLD_UP        →  FAILED                  (ficou dentro por breakoutFailedTimeoutMs)
//   (idem para DOWN)
// =============================================================================

class BreakoutStateEngine {
public:
    explicit BreakoutStateEngine(const Config& config);

    // Atualiza o estado do breakout para o ativo do snapshot.
    BreakoutState update(const MarketSnapshot& snapshot,
                         const PriceStructureSnapshot& structure);

    // Marca o breakout do ativo como consumido, impedindo nova entrada
    // no mesmo breakout.
    void markEntryConsumed(const std::string& exchange, const std::string& symbol);

private:
    const Config& config_;

    std::unordered_map<std::string, BreakoutState> stateByKey_;

    std::string makeKey(const std::string& exchange, const std::string& symbol) const;
};
