#pragma once

#include <string>
#include <unordered_map>

#include "../domain/MarketSnapshot.h"
#include "../domain/SignalResult.h"
#include "../domain/RegimeSnapshot.h"
#include "../domain/PriceStructureSnapshot.h"
#include "../domain/BreakoutState.h"

class SignalDiagnosticsCollector {
public:
    void onSignal(const MarketSnapshot& snapshot,
                  const SignalResult& signal,
                  const RegimeSnapshot& regime,
                  const PriceStructureSnapshot& structure,
                  const BreakoutState& breakoutState,
                  bool persistenceApproved,
                  bool executionApproved);

    long getTotalSignals() const;
    long getValidSignals() const;
    long getHoldSignals() const;

    long getPersistenceApprovedCount() const;
    long getExecutionApprovedCount() const;

    long getBreakoutUpCount() const;
    long getBreakoutDownCount() const;
    long getBreakoutActiveCount() const;
    long getBreakoutReturnedInsideCount() const;

    long getPhaseIdleCount() const;
    long getPhaseBreakUpCount() const;
    long getPhaseBreakDownCount() const;
    long getPhaseHoldUpCount() const;
    long getPhaseHoldDownCount() const;
    long getPhaseConfirmedUpCount() const;
    long getPhaseConfirmedDownCount() const;
    long getPhaseFailedCount() const;

    double getAverageConfidence() const;
    double getMaxConfidence() const;

    double getAverageExpectedMoveBps() const;
    double getMaxExpectedMoveBps() const;

    const std::unordered_map<std::string, long>& getReasonCounts() const;

    void printSummary() const;

private:
    long totalSignals_{0};
    long validSignals_{0};
    long holdSignals_{0};

    long persistenceApprovedCount_{0};
    long executionApprovedCount_{0};

    long breakoutUpCount_{0};
    long breakoutDownCount_{0};
    long breakoutActiveCount_{0};
    long breakoutReturnedInsideCount_{0};

    long phaseIdleCount_{0};
    long phaseBreakUpCount_{0};
    long phaseBreakDownCount_{0};
    long phaseHoldUpCount_{0};
    long phaseHoldDownCount_{0};
    long phaseConfirmedUpCount_{0};
    long phaseConfirmedDownCount_{0};
    long phaseFailedCount_{0};

    double confidenceSum_{0.0};
    double confidenceMax_{0.0};

    double expectedMoveBpsSum_{0.0};
    double expectedMoveBpsMax_{0.0};

    std::unordered_map<std::string, long> reasonCounts_;

    void countBreakoutPhase(const BreakoutState& breakoutState);
};