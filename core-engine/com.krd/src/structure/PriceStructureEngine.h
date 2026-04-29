#pragma once

#include <deque>
#include <string>
#include <unordered_map>

#include "../config/Config.h"
#include "../domain/MarketSnapshot.h"
#include "../domain/PriceStructureSnapshot.h"

// =============================================================================
// PriceStructureEngine
//
// Responsabilidade: manter histórico de preços por ativo e calcular a estrutura
// recente de preço (faixa, breakout, distância do rompimento).
//
// A faixa é calculada com o histórico ANTERIOR ao preço atual, garantindo que
// o breakout seja comparado contra uma estrutura já formada.
// =============================================================================

class PriceStructureEngine {
public:
    explicit PriceStructureEngine(const Config& config);

    // Atualiza o histórico e retorna a estrutura de preço atual.
    PriceStructureSnapshot update(const MarketSnapshot& snapshot);

private:
    struct PricePoint {
        double price{0.0};
        std::int64_t timestampMs{0};
    };

    const Config& config_;

    std::unordered_map<std::string, std::deque<PricePoint>> historyByKey_;

    std::string makeKey(const std::string& exchange, const std::string& symbol) const;

    // Remove pontos do histórico mais antigos que config_.priceStructureWindowMs.
    void trimHistory(std::deque<PricePoint>& history, std::int64_t nowMs) const;

    double calculateRangeBps(double lowPrice, double highPrice) const;
    double calculateBreakoutDistanceBps(double referencePrice, double currentPrice) const;
};
