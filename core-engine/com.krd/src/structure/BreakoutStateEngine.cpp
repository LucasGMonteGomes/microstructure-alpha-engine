#include "BreakoutStateEngine.h"

#include <algorithm>

BreakoutState BreakoutStateEngine::update(const MarketSnapshot& snapshot,
                                          const PriceStructureSnapshot& structure) {
    const std::string key = makeKey(snapshot.exchange, snapshot.symbol);
    BreakoutState& state = stateByKey_[key];

    state.exchange = snapshot.exchange;
    state.symbol = snapshot.symbol;

    if (!state.active) {
        state.phase = BreakoutPhase::IDLE;
    }

    if (structure.isBreakoutUp) {
        if (!state.active || state.phase == BreakoutPhase::IDLE || state.phase == BreakoutPhase::FAILED) {
            state.phase = BreakoutPhase::BREAK_UP;
            state.breakPrice = snapshot.midPrice;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = snapshot.midPrice;
            state.lowestPriceAfterBreak = snapshot.midPrice;
            state.returnedInsideRange = false;
            state.active = true;
            state.breakTimestampMs = snapshot.timestampMs;
            return state;
        }

        if (state.phase == BreakoutPhase::BREAK_UP || state.phase == BreakoutPhase::HOLD_UP) {
            state.phase = BreakoutPhase::CONFIRMED_UP;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = std::max(state.highestPriceAfterBreak, snapshot.midPrice);
            state.lowestPriceAfterBreak = std::min(state.lowestPriceAfterBreak, snapshot.midPrice);
            return state;
        }
    }

    if (structure.isBreakoutDown) {
        if (!state.active || state.phase == BreakoutPhase::IDLE || state.phase == BreakoutPhase::FAILED) {
            state.phase = BreakoutPhase::BREAK_DOWN;
            state.breakPrice = snapshot.midPrice;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = snapshot.midPrice;
            state.lowestPriceAfterBreak = snapshot.midPrice;
            state.returnedInsideRange = false;
            state.active = true;
            state.breakTimestampMs = snapshot.timestampMs;
            return state;
        }

        if (state.phase == BreakoutPhase::BREAK_DOWN || state.phase == BreakoutPhase::HOLD_DOWN) {
            state.phase = BreakoutPhase::CONFIRMED_DOWN;
            state.lastConfirmedPrice = snapshot.midPrice;
            state.highestPriceAfterBreak = std::max(state.highestPriceAfterBreak, snapshot.midPrice);
            state.lowestPriceAfterBreak = std::min(state.lowestPriceAfterBreak, snapshot.midPrice);
            return state;
        }
    }

    if (state.active) {
        state.highestPriceAfterBreak = std::max(state.highestPriceAfterBreak, snapshot.midPrice);
        state.lowestPriceAfterBreak = std::min(state.lowestPriceAfterBreak, snapshot.midPrice);

        if (snapshot.midPrice <= structure.recentHigh && snapshot.midPrice >= structure.recentLow) {
            state.returnedInsideRange = true;

            if (state.phase == BreakoutPhase::BREAK_UP || state.phase == BreakoutPhase::CONFIRMED_UP) {
                state.phase = BreakoutPhase::HOLD_UP;
            } else if (state.phase == BreakoutPhase::BREAK_DOWN || state.phase == BreakoutPhase::CONFIRMED_DOWN) {
                state.phase = BreakoutPhase::HOLD_DOWN;
            }
        }

        if (state.returnedInsideRange) {
            const std::int64_t ageMs = snapshot.timestampMs - state.breakTimestampMs;
            if (ageMs > 5000) {
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