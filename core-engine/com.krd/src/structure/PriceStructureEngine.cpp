#include "PriceStructureEngine.h"

#include <algorithm>

PriceStructureEngine::PriceStructureEngine(const Config& config)
    : config_(config) {}

PriceStructureSnapshot PriceStructureEngine::update(const MarketSnapshot& snapshot) {
    const std::string key = makeKey(snapshot.exchange, snapshot.symbol);
    auto& history = historyByKey_[key];

    // Primeiro, limpa o histórico antigo
    trimHistory(history, snapshot.timestampMs);

    PriceStructureSnapshot structure;
    structure.exchange = snapshot.exchange;
    structure.symbol = snapshot.symbol;
    structure.timestampMs = snapshot.timestampMs;

    // Se ainda não há histórico suficiente, só armazena o preço atual e retorna
    if (history.empty()) {
        history.push_back(PricePoint{snapshot.midPrice, snapshot.timestampMs});

        structure.recentHigh = snapshot.midPrice;
        structure.recentLow = snapshot.midPrice;
        structure.midRangePrice = snapshot.midPrice;
        structure.rangeBps = 0.0;
        structure.breakoutDistanceBps = 0.0;
        structure.isCompressed = true;
        structure.isBreakoutUp = false;
        structure.isBreakoutDown = false;

        return structure;
    }

    // Calcula a faixa ANTERIOR sem incluir o preço atual
    double recentHigh = history.front().price;
    double recentLow = history.front().price;

    for (const auto& point : history) {
        recentHigh = std::max(recentHigh, point.price);
        recentLow = std::min(recentLow, point.price);
    }

    structure.recentHigh = recentHigh;
    structure.recentLow = recentLow;
    structure.midRangePrice = (recentHigh + recentLow) / 2.0;
    structure.rangeBps = calculateRangeBps(recentLow, recentHigh);

    // Compressão: faixa curta o suficiente para um breakout ser relevante
    structure.isCompressed = structure.rangeBps <= 8.0;

    // Agora sim compara o preço atual contra a faixa anterior
    if (snapshot.midPrice > recentHigh) {
        structure.isBreakoutUp = true;
        structure.isBreakoutDown = false;
        structure.breakoutDistanceBps =
            calculateBreakoutDistanceBps(recentHigh, snapshot.midPrice);
    } else if (snapshot.midPrice < recentLow) {
        structure.isBreakoutUp = false;
        structure.isBreakoutDown = true;
        structure.breakoutDistanceBps =
            std::abs(calculateBreakoutDistanceBps(recentLow, snapshot.midPrice));
    } else {
        structure.isBreakoutUp = false;
        structure.isBreakoutDown = false;
        structure.breakoutDistanceBps = 0.0;
    }

    // Só depois de calcular a estrutura, adiciona o preço atual ao histórico
    history.push_back(PricePoint{snapshot.midPrice, snapshot.timestampMs});
    trimHistory(history, snapshot.timestampMs);

    return structure;
}

std::string PriceStructureEngine::makeKey(const std::string& exchange,
                                          const std::string& symbol) const {
    return exchange + "|" + symbol;
}

void PriceStructureEngine::trimHistory(std::deque<PricePoint>& history,
                                       std::int64_t nowMs) const {
    const std::int64_t windowMs = 30 * 1000;

    while (!history.empty()) {
        const auto ageMs = nowMs - history.front().timestampMs;
        if (ageMs <= windowMs) {
            break;
        }
        history.pop_front();
    }
}

double PriceStructureEngine::calculateRangeBps(double lowPrice, double highPrice) const {
    if (lowPrice <= 0.0 || highPrice <= 0.0 || highPrice < lowPrice) {
        return 0.0;
    }

    const double midPrice = (lowPrice + highPrice) / 2.0;
    if (midPrice <= 0.0) {
        return 0.0;
    }

    return ((highPrice - lowPrice) / midPrice) * 10000.0;
}

double PriceStructureEngine::calculateBreakoutDistanceBps(double referencePrice,
                                                          double currentPrice) const {
    if (referencePrice <= 0.0 || currentPrice <= 0.0) {
        return 0.0;
    }

    return ((currentPrice - referencePrice) / referencePrice) * 10000.0;
}