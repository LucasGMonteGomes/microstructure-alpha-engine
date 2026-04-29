#include "PaperTradeEngine.h"

PaperTradeEngine::PaperTradeEngine(const Config& config)
    : config_(config) {}

bool PaperTradeEngine::hasOpenPosition() const {
    return currentPosition_.isOpen;
}

const Position& PaperTradeEngine::getOpenPosition() const {
    return currentPosition_;
}

bool PaperTradeEngine::tryOpenPosition(const MarketSnapshot& snapshot, const SignalResult& signal) {
    if (currentPosition_.isOpen) {
        return false;
    }

    if (!signal.isValid || signal.side == SignalSide::HOLD) {
        return false;
    }

    currentPosition_.isOpen = true;
    currentPosition_.exchange = snapshot.exchange;
    currentPosition_.symbol = snapshot.symbol;
    currentPosition_.side = signal.side;
    currentPosition_.entryPrice = snapshot.midPrice;
    currentPosition_.targetPrice = buildTargetPrice(snapshot, signal.side);
    currentPosition_.stopPrice = buildStopPrice(snapshot, signal.side);
    currentPosition_.entryTimestampMs = snapshot.timestampMs;
    currentPosition_.timeoutTimestampMs = snapshot.timestampMs + config_.timeoutMs;
    currentPosition_.entryImbalance = snapshot.imbalance;
    currentPosition_.entryConfidence = signal.confidence;
    currentPosition_.expectedMoveBps = signal.expectedMoveBps;
    currentPosition_.exitReason = ExitReason::NONE;

    return true;
}

std::optional<TradeResult> PaperTradeEngine::update(const MarketSnapshot& snapshot,
                                                    const SignalResult& latestSignal) {
    if (!currentPosition_.isOpen) {
        return std::nullopt;
    }

    if (snapshot.exchange != currentPosition_.exchange || snapshot.symbol != currentPosition_.symbol) {
        return std::nullopt;
    }

    const double currentPrice = snapshot.midPrice;

    if (currentPosition_.side == SignalSide::LONG) {
        if (currentPrice >= currentPosition_.targetPrice) {
            return closePosition(snapshot, ExitReason::TAKE_PROFIT);
        }
        if (currentPrice <= currentPosition_.stopPrice) {
            return closePosition(snapshot, ExitReason::STOP_LOSS);
        }
    } else if (currentPosition_.side == SignalSide::SHORT) {
        if (currentPrice <= currentPosition_.targetPrice) {
            return closePosition(snapshot, ExitReason::TAKE_PROFIT);
        }
        if (currentPrice >= currentPosition_.stopPrice) {
            return closePosition(snapshot, ExitReason::STOP_LOSS);
        }
    }

    // Invalidação por fluxo desativada temporariamente.
    // O objetivo agora é deixar a operação chegar em TAKE_PROFIT, STOP_LOSS ou TIMEOUT.
    /*
    if (shouldExitEarly(latestSignal)) {
        return closePosition(snapshot, ExitReason::INVALIDATION);
    }
    */

    if (snapshot.timestampMs >= currentPosition_.timeoutTimestampMs) {
        return closePosition(snapshot, ExitReason::TIMEOUT);
    }

    return std::nullopt;
}

std::optional<TradeResult> PaperTradeEngine::forceClosePosition(const MarketSnapshot& snapshot,
                                                                ExitReason reason) {
    if (!currentPosition_.isOpen) {
        return std::nullopt;
    }

    if (snapshot.exchange != currentPosition_.exchange ||
        snapshot.symbol != currentPosition_.symbol) {
        return std::nullopt;
        }

    return closePosition(snapshot, reason);
}

double PaperTradeEngine::calculateGrossPnlPct(const Position& position, double exitPrice) const {
    if (position.entryPrice <= 0.0) {
        return 0.0;
    }

    if (position.side == SignalSide::LONG) {
        return ((exitPrice - position.entryPrice) / position.entryPrice) * 100.0;
    }

    if (position.side == SignalSide::SHORT) {
        return ((position.entryPrice - exitPrice) / position.entryPrice) * 100.0;
    }

    return 0.0;
}

double PaperTradeEngine::calculateNetPnlPct(double grossPnlPct) const {
    const double totalFees = config_.feePctPerSide * 2.0;
    const double totalSlippage = config_.slippagePctPerSide * 2.0;
    return grossPnlPct - totalFees - totalSlippage;
}

double PaperTradeEngine::buildTargetPrice(const MarketSnapshot& snapshot, SignalSide side) const {
    const double factor = config_.targetPct / 100.0;

    if (side == SignalSide::LONG) {
        return snapshot.midPrice * (1.0 + factor);
    }

    if (side == SignalSide::SHORT) {
        return snapshot.midPrice * (1.0 - factor);
    }

    return snapshot.midPrice;
}

double PaperTradeEngine::buildStopPrice(const MarketSnapshot& snapshot, SignalSide side) const {
    const double factor = config_.stopPct / 100.0;

    if (side == SignalSide::LONG) {
        return snapshot.midPrice * (1.0 - factor);
    }

    if (side == SignalSide::SHORT) {
        return snapshot.midPrice * (1.0 + factor);
    }

    return snapshot.midPrice;
}

bool PaperTradeEngine::shouldExitEarly(const SignalResult& latestSignal) const {
    if (!currentPosition_.isOpen) {
        return false;
    }

    if (currentPosition_.side == SignalSide::LONG) {
        if (latestSignal.flowBias < 0.05) {
            return true;
        }
    }

    if (currentPosition_.side == SignalSide::SHORT) {
        if (latestSignal.flowBias > -0.05) {
            return true;
        }
    }

    return false;
}

TradeResult PaperTradeEngine::closePosition(const MarketSnapshot& snapshot, ExitReason reason) {
    TradeResult result;
    result.exchange = currentPosition_.exchange;
    result.symbol = currentPosition_.symbol;
    result.side = currentPosition_.side;
    result.entryPrice = currentPosition_.entryPrice;
    result.exitPrice = snapshot.midPrice;
    result.entryTimestampMs = currentPosition_.entryTimestampMs;
    result.exitTimestampMs = snapshot.timestampMs;
    result.exitReason = reason;
    result.grossPnlPct = calculateGrossPnlPct(currentPosition_, snapshot.midPrice);
    result.feePct = config_.feePctPerSide * 2.0;
    result.slippagePct = config_.slippagePctPerSide * 2.0;
    result.netPnlPct = calculateNetPnlPct(result.grossPnlPct);
    result.entryImbalance = currentPosition_.entryImbalance;
    result.entryConfidence = currentPosition_.entryConfidence;

    currentPosition_ = Position{};
    return result;
}