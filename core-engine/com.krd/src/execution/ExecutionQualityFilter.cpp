#include "ExecutionQualityFilter.h"

ExecutionQualityFilter::ExecutionQualityFilter(const Config& config)
    : config_(config) {}

bool ExecutionQualityFilter::shouldAllowEntry(const MarketSnapshot& snapshot,
                                              const SignalResult& signal) const {
    if (!signal.isValid || signal.side == SignalSide::HOLD) {
        return false;
    }

    const double requiredMoveBps = estimateRequiredMoveBps(snapshot);

    return signal.expectedMoveBps >= requiredMoveBps;
}

double ExecutionQualityFilter::estimateTotalCostBps(const MarketSnapshot& snapshot) const {
    const double feeBps = config_.feePctPerSide * 2.0 * 100.0;
    const double slippageBps = config_.slippagePctPerSide * 2.0 * 100.0;
    const double spreadPenaltyBps = snapshot.spreadBps;

    return feeBps + slippageBps + spreadPenaltyBps;
}

double ExecutionQualityFilter::estimateRequiredMoveBps(
    const MarketSnapshot& snapshot) const {

    const double totalCostBps = estimateTotalCostBps(snapshot);
    return totalCostBps + config_.executionSafetyBufferBps;
}