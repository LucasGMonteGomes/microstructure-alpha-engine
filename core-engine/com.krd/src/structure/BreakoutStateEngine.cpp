#include "BreakoutStateEngine.h"

#include <algorithm>

BreakoutStateEngine::BreakoutStateEngine(const Config& config)
    : config_(config) {}

BreakoutState BreakoutStateEngine::update(const MarketSnapshot& snapshot,
                                          const PriceStructureSnapshot& structure) {
    const std::string key = makeKey(snapshot.exchange, snapshot.symbol);
    BreakoutState& state = stateByKey_[key];

    state.exchange = snapshot.exchange;
    state.symbol = snapshot.symbol;

    if (!state.active) {
        state.phase = BreakoutPhase::IDLE;
    }

    // ------------------------------------------------------------------
    // Novo breakout para cima
    // ------------------------------------------------------------------
    if (structure.isBreakoutUp) {
        // Inicia novo breakout se estava inativo ou falhou
        if (!state.active ||
            state.phase == BreakoutPhase::IDLE ||
            state.phase == BreakoutPhase::FAILED) {

            state.phase = BreakoutPhase::BREAK_UP;
            state.breakPrice = snapshot.midPrice;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = snapshot.midPrice;
            state.lowestPriceAfterBreak = snapshot.midPrice;
            state.returnedInsideRange = false;
            state.active = true;
            state.entryConsumed = false;
            state.breakTimestampMs = snapshot.timestampMs;
            return state;
        }

        // Confirma breakout que estava em BREAK ou HOLD
        if (state.phase == BreakoutPhase::BREAK_UP ||
            state.phase == BreakoutPhase::HOLD_UP) {

            state.phase = BreakoutPhase::CONFIRMED_UP;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak =
                std::max(state.highestPriceAfterBreak, snapshot.midPrice);
            state.lowestPriceAfterBreak =
                std::min(state.lowestPriceAfterBreak, snapshot.midPrice);
            return state;
        }
    }

    // ------------------------------------------------------------------
    // Novo breakout para baixo
    // ------------------------------------------------------------------
    if (structure.isBreakoutDown) {
        if (!state.active ||
            state.phase == BreakoutPhase::IDLE ||
            state.phase == BreakoutPhase::FAILED) {

            state.phase = BreakoutPhase::BREAK_DOWN;
            state.breakPrice = snapshot.midPrice;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = snapshot.midPrice;
            state.lowestPriceAfterBreak = snapshot.midPrice;
            state.returnedInsideRange = false;
            state.active = true;
            state.entryConsumed = false;
            state.breakTimestampMs = snapshot.timestampMs;
            return state;
        }

        if (state.phase == BreakoutPhase::BREAK_DOWN ||
            state.phase == BreakoutPhase::HOLD_DOWN) {

            state.phase = BreakoutPhase::CONFIRMED_DOWN;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak =
                std::max(state.highestPriceAfterBreak, snapshot.midPrice);
            state.lowestPriceAfterBreak =
                std::min(state.lowestPriceAfterBreak, snapshot.midPrice);
            return state;
        }
    }

    // ------------------------------------------------------------------
    // Acompanhamento de breakout ativo (sem novo breakout neste snapshot)
    // ------------------------------------------------------------------
    if (state.active) {
        state.highestPriceAfterBreak =
            std::max(state.highestPriceAfterBreak, snapshot.midPrice);
        state.lowestPriceAfterBreak =
            std::min(state.lowestPriceAfterBreak, snapshot.midPrice);

        // Preço voltou para dentro da faixa anterior
        const bool insideRange =
            snapshot.midPrice <= structure.recentHigh &&
            snapshot.midPrice >= structure.recentLow;

        if (insideRange) {
            state.returnedInsideRange = true;

            if (state.phase == BreakoutPhase::BREAK_UP ||
                state.phase == BreakoutPhase::CONFIRMED_UP) {
                state.phase = BreakoutPhase::HOLD_UP;
            } else if (state.phase == BreakoutPhase::BREAK_DOWN ||
                       state.phase == BreakoutPhase::CONFIRMED_DOWN) {
                state.phase = BreakoutPhase::HOLD_DOWN;
            }
        }

        // Se o preço ficou dentro por mais do que o timeout configurado → FAILED
        if (state.returnedInsideRange) {
            const std::int64_t ageMs = snapshot.timestampMs - state.breakTimestampMs;
            if (ageMs > config_.breakoutFailedTimeoutMs) {
                state.phase = BreakoutPhase::FAILED;
                state.active = false;
            }
        }
    }

    return state;
}

std::string BreakoutStateEngine::makeKey(const std::string& exchange,
                                         const std::string& symbol) const {
    return exchange + "|" + symbol;
}

void BreakoutStateEngine::markEntryConsumed(const std::string& exchange,
                                            const std::string& symbol) {
    const std::string key = makeKey(exchange, symbol);
    auto it = stateByKey_.find(key);

    if (it == stateByKey_.end()) {
        return;
    }

    it->second.entryConsumed = true;
}
