#include "TestRunPaths.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace {
    std::string buildTimestampString() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);

        std::tm localTm{};
#ifdef _WIN32
        localtime_s(&localTm, &nowTime);
#else
        localtime_r(&nowTime, &localTm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&localTm, "%Y-%m-%d_%H-%M-%S");
        return oss.str();
    }

    long buildDurationMinutes(long testDurationMs) {
        return testDurationMs / (60L * 1000L);
    }

    std::filesystem::path resolveProjectTestRunsDir() {
        const std::filesystem::path currentDir = std::filesystem::current_path();
        const std::string currentDirName = currentDir.filename().string();

        if (currentDirName.rfind("cmake-build", 0) == 0) {
            return currentDir.parent_path() / "test_runs";
        }

        return currentDir / "test_runs";
    }
}

TestRunPaths TestRunPathsBuilder::build(long testDurationMs) {
    const std::string timestamp = buildTimestampString();
    const long durationMinutes = buildDurationMinutes(testDurationMs);

    const std::string runFolderName =
        timestamp + "_" + std::to_string(durationMinutes) + "min";

    const std::filesystem::path baseDir = resolveProjectTestRunsDir();
    const std::filesystem::path runDir = baseDir / runFolderName;

    std::filesystem::create_directories(runDir);

    TestRunPaths paths;
    paths.runDirectory = runDir.string();
    paths.tradeResultsCsvPath = (runDir / "trade_results.csv").string();
    paths.testSummaryCsvPath = (runDir / "test_summary.csv").string();
    paths.diagnosticSummaryCsvPath = (runDir / "diagnostic_summary.csv").string();

    return paths;
}