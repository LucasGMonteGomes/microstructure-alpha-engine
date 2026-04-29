#pragma once

#include <string>

struct TestRunPaths {
    std::string runDirectory;
    std::string tradeResultsCsvPath;
    std::string testSummaryCsvPath;
    std::string diagnosticSummaryCsvPath;
};

class TestRunPathsBuilder {
public:
    static TestRunPaths build(long testDurationMs);
};