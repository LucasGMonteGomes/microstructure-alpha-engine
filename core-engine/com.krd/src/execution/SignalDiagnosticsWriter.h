#pragma once

#include <string>

#include "SignalDiagnosticsCollector.h"

class SignalDiagnosticsWriter {
public:
    static bool writeSummary(const std::string& filePath,
                             const SignalDiagnosticsCollector& diagnosticsCollector);
};