#include "SignalDiagnosticsCollector.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

void SignalDiagnosticsCollector::onSignal(const MarketSnapshot& snapshot,
                                          const SignalResult& signal,
                                          const RegimeSnapshot& regime,
                                          const PriceStructureSnapshot& structure,
                                          const BreakoutState& breakoutState,
                                          bool persistenceApproved,
                                          bool executionApproved) {
    (void)snapshot;
    (void)regime;

    totalSignals_++;

    if (signal.isValid) {
        validSignals_++;
    }

    if (signal.side == SignalSide::HOLD) {
        holdSignals_++;
    }

    if (persistenceApproved) {
        persistenceApprovedCount_++;
    }

    if (executionApproved) {
        executionApprovedCount_++;
    }

    if (structure.isBreakoutUp) {
        breakoutUpCount_++;
    }

    if (structure.isBreakoutDown) {
        breakoutDownCount_++;
    }

    if (breakoutState.active) {
        breakoutActiveCount_++;
    }

    if (breakoutState.returnedInsideRange) {
        breakoutReturnedInsideCount_++;
    }

    countBreakoutPhase(breakoutState);

    confidenceSum_ += signal.confidence;
    confidenceMax_ = std::max(confidenceMax_, signal.confidence);

    expectedMoveBpsSum_ += signal.expectedMoveBps;
    expectedMoveBpsMax_ = std::max(expectedMoveBpsMax_, signal.expectedMoveBps);

    reasonCounts_[signal.reason]++;
}

long SignalDiagnosticsCollector::getTotalSignals() const {
    return totalSignals_;
}

long SignalDiagnosticsCollector::getValidSignals() const {
    return validSignals_;
}

long SignalDiagnosticsCollector::getHoldSignals() const {
    return holdSignals_;
}

long SignalDiagnosticsCollector::getPersistenceApprovedCount() const {
    return persistenceApprovedCount_;
}

long SignalDiagnosticsCollector::getExecutionApprovedCount() const {
    return executionApprovedCount_;
}

long SignalDiagnosticsCollector::getBreakoutUpCount() const {
    return breakoutUpCount_;
}

long SignalDiagnosticsCollector::getBreakoutDownCount() const {
    return breakoutDownCount_;
}

long SignalDiagnosticsCollector::getBreakoutActiveCount() const {
    return breakoutActiveCount_;
}

long SignalDiagnosticsCollector::getBreakoutReturnedInsideCount() const {
    return breakoutReturnedInsideCount_;
}

long SignalDiagnosticsCollector::getPhaseIdleCount() const {
    return phaseIdleCount_;
}

long SignalDiagnosticsCollector::getPhaseBreakUpCount() const {
    return phaseBreakUpCount_;
}

long SignalDiagnosticsCollector::getPhaseBreakDownCount() const {
    return phaseBreakDownCount_;
}

long SignalDiagnosticsCollector::getPhaseHoldUpCount() const {
    return phaseHoldUpCount_;
}

long SignalDiagnosticsCollector::getPhaseHoldDownCount() const {
    return phaseHoldDownCount_;
}

long SignalDiagnosticsCollector::getPhaseConfirmedUpCount() const {
    return phaseConfirmedUpCount_;
}

long SignalDiagnosticsCollector::getPhaseConfirmedDownCount() const {
    return phaseConfirmedDownCount_;
}

long SignalDiagnosticsCollector::getPhaseFailedCount() const {
    return phaseFailedCount_;
}

double SignalDiagnosticsCollector::getAverageConfidence() const {
    if (totalSignals_ <= 0) {
        return 0.0;
    }

    return confidenceSum_ / static_cast<double>(totalSignals_);
}

double SignalDiagnosticsCollector::getMaxConfidence() const {
    return confidenceMax_;
}

double SignalDiagnosticsCollector::getAverageExpectedMoveBps() const {
    if (totalSignals_ <= 0) {
        return 0.0;
    }

    return expectedMoveBpsSum_ / static_cast<double>(totalSignals_);
}

double SignalDiagnosticsCollector::getMaxExpectedMoveBps() const {
    return expectedMoveBpsMax_;
}

const std::unordered_map<std::string, long>& SignalDiagnosticsCollector::getReasonCounts() const {
    return reasonCounts_;
}

void SignalDiagnosticsCollector::printSummary() const {
    std::cout << "\n[DIAGNOSTIC SUMMARY]\n"
              << "totalSignals=" << getTotalSignals() << "\n"
              << "validSignals=" << getValidSignals() << "\n"
              << "holdSignals=" << getHoldSignals() << "\n"
              << "persistenceApprovedCount=" << getPersistenceApprovedCount() << "\n"
              << "executionApprovedCount=" << getExecutionApprovedCount() << "\n"
              << "breakoutUpCount=" << getBreakoutUpCount() << "\n"
              << "breakoutDownCount=" << getBreakoutDownCount() << "\n"
              << "breakoutActiveCount=" << getBreakoutActiveCount() << "\n"
              << "breakoutReturnedInsideCount=" << getBreakoutReturnedInsideCount() << "\n"
              << "phaseIdleCount=" << getPhaseIdleCount() << "\n"
              << "phaseBreakUpCount=" << getPhaseBreakUpCount() << "\n"
              << "phaseBreakDownCount=" << getPhaseBreakDownCount() << "\n"
              << "phaseHoldUpCount=" << getPhaseHoldUpCount() << "\n"
              << "phaseHoldDownCount=" << getPhaseHoldDownCount() << "\n"
              << "phaseConfirmedUpCount=" << getPhaseConfirmedUpCount() << "\n"
              << "phaseConfirmedDownCount=" << getPhaseConfirmedDownCount() << "\n"
              << "phaseFailedCount=" << getPhaseFailedCount() << "\n"
              << std::fixed << std::setprecision(6)
              << "avgConfidence=" << getAverageConfidence() << "\n"
              << "maxConfidence=" << getMaxConfidence() << "\n"
              << "avgExpectedMoveBps=" << getAverageExpectedMoveBps() << "\n"
              << "maxExpectedMoveBps=" << getMaxExpectedMoveBps() << "\n";

    std::vector<std::pair<std::string, long>> reasons(reasonCounts_.begin(), reasonCounts_.end());

    std::sort(reasons.begin(), reasons.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    std::cout << "\n[DIAGNOSTIC REASONS]\n";

    for (const auto& item : reasons) {
        std::cout << item.first << "=" << item.second << "\n";
    }

    std::cout << std::endl;
}

void SignalDiagnosticsCollector::countBreakoutPhase(const BreakoutState& breakoutState) {
    switch (breakoutState.phase) {
        case BreakoutPhase::IDLE:
            phaseIdleCount_++;
            break;
        case BreakoutPhase::BREAK_UP:
            phaseBreakUpCount_++;
            break;
        case BreakoutPhase::BREAK_DOWN:
            phaseBreakDownCount_++;
            break;
        case BreakoutPhase::HOLD_UP:
            phaseHoldUpCount_++;
            break;
        case BreakoutPhase::HOLD_DOWN:
            phaseHoldDownCount_++;
            break;
        case BreakoutPhase::CONFIRMED_UP:
            phaseConfirmedUpCount_++;
            break;
        case BreakoutPhase::CONFIRMED_DOWN:
            phaseConfirmedDownCount_++;
            break;
        case BreakoutPhase::FAILED:
            phaseFailedCount_++;
            break;
    }
}