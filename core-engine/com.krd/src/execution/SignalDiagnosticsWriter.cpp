#include "SignalDiagnosticsWriter.h"

#include <fstream>
#include <vector>
#include <algorithm>

bool SignalDiagnosticsWriter::writeSummary(
    const std::string& filePath,
    const SignalDiagnosticsCollector& diagnosticsCollector
) {
    std::ofstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    file << "metric,value\n";

    file << "total_signals," << diagnosticsCollector.getTotalSignals() << "\n";
    file << "valid_signals," << diagnosticsCollector.getValidSignals() << "\n";
    file << "hold_signals," << diagnosticsCollector.getHoldSignals() << "\n";

    file << "persistence_approved_count," << diagnosticsCollector.getPersistenceApprovedCount() << "\n";
    file << "execution_approved_count," << diagnosticsCollector.getExecutionApprovedCount() << "\n";

    file << "breakout_up_count," << diagnosticsCollector.getBreakoutUpCount() << "\n";
    file << "breakout_down_count," << diagnosticsCollector.getBreakoutDownCount() << "\n";
    file << "breakout_active_count," << diagnosticsCollector.getBreakoutActiveCount() << "\n";
    file << "breakout_returned_inside_count," << diagnosticsCollector.getBreakoutReturnedInsideCount() << "\n";

    file << "phase_idle_count," << diagnosticsCollector.getPhaseIdleCount() << "\n";
    file << "phase_break_up_count," << diagnosticsCollector.getPhaseBreakUpCount() << "\n";
    file << "phase_break_down_count," << diagnosticsCollector.getPhaseBreakDownCount() << "\n";
    file << "phase_hold_up_count," << diagnosticsCollector.getPhaseHoldUpCount() << "\n";
    file << "phase_hold_down_count," << diagnosticsCollector.getPhaseHoldDownCount() << "\n";
    file << "phase_confirmed_up_count," << diagnosticsCollector.getPhaseConfirmedUpCount() << "\n";
    file << "phase_confirmed_down_count," << diagnosticsCollector.getPhaseConfirmedDownCount() << "\n";
    file << "phase_failed_count," << diagnosticsCollector.getPhaseFailedCount() << "\n";

    file << "avg_confidence," << diagnosticsCollector.getAverageConfidence() << "\n";
    file << "max_confidence," << diagnosticsCollector.getMaxConfidence() << "\n";
    file << "avg_expected_move_bps," << diagnosticsCollector.getAverageExpectedMoveBps() << "\n";
    file << "max_expected_move_bps," << diagnosticsCollector.getMaxExpectedMoveBps() << "\n";

    std::vector<std::pair<std::string, long>> reasons(
        diagnosticsCollector.getReasonCounts().begin(),
        diagnosticsCollector.getReasonCounts().end()
    );

    std::sort(reasons.begin(), reasons.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    file << "\nreason,count\n";

    for (const auto& item : reasons) {
        file << "\"" << item.first << "\"," << item.second << "\n";
    }

    return true;
}