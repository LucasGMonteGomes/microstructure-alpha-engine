cmake_minimum_required(VERSION 3.20)
project(core_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ixwebsocket CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cppzmq CONFIG REQUIRED)

add_executable(core_engine
        com.krd/src/main.cpp
        com.krd/src/strategy/signal_engine/SignalEngineLite.cpp
        com.krd/src/execution/PaperTradeEngine.cpp
        com.krd/src/messaging/TradePublisher.cpp

        com.krd/src/domain/TradeTick.h
        com.krd/src/domain/FlowSnapshot.h
        com.krd/src/domain/SignalCandidate.h
        com.krd/src/domain/RegimeSnapshot.h

        com.krd/src/flow/AggressionTracker.cpp
        com.krd/src/flow/AggressionTracker.h

        com.krd/src/persistence/SignalPersistenceFilter.cpp
        com.krd/src/persistence/SignalPersistenceFilter.h

        com.krd/src/regime/RegimeFilter.cpp
        com.krd/src/regime/RegimeFilter.h

        com.krd/src/strategy/common/ISignalStrategy.cpp
        com.krd/src/strategy/common/ISignalStrategy.h
        com.krd/src/strategy/common/StrategyContext.cpp
        com.krd/src/strategy/common/StrategyContext.h

        com.krd/src/execution/ExecutionQualityFilter.cpp
        com.krd/src/execution/ExecutionQualityFilter.h
        com.krd/src/execution/TradeStatsCollector.cpp
        com.krd/src/execution/TradeStatsCollector.h
        com.krd/src/execution/TradeCsvWriter.cpp
        com.krd/src/execution/TradeCsvWriter.h
        com.krd/src/execution/TestSummaryWriter.cpp
        com.krd/src/execution/TestSummaryWriter.h
        com.krd/src/execution/TestRunPaths.cpp
        com.krd/src/execution/TestRunPaths.h

        com.krd/src/strategy/regime_hold_continuation/RegimeHoldContinuationStrategy.cpp
        com.krd/src/strategy/regime_hold_continuation/RegimeHoldContinuationStrategy.h
        com.krd/src/domain/PriceStructureSnapshot.h
        com.krd/src/domain/BreakoutPhase.h
        com.krd/src/domain/BreakoutState.h
        com.krd/src/structure/PriceStructureEngine.h
        com.krd/src/structure/BreakoutStateEngine.h
        com.krd/src/structure/PriceStructureEngine.cpp
        com.krd/src/structure/BreakoutStateEngine.cpp
        com.krd/src/strategy/breakout_confirmation_v2/BreakoutConfirmationV2Strategy.cpp
        com.krd/src/strategy/breakout_confirmation_v2/BreakoutConfirmationV2Strategy.h
        com.krd/src/execution/SignalDiagnosticsCollector.cpp
        com.krd/src/execution/SignalDiagnosticsCollector.h
        com.krd/src/execution/SignalDiagnosticsWriter.cpp
        com.krd/src/execution/SignalDiagnosticsWriter.h
)

target_include_directories(core_engine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(core_engine PRIVATE
        ixwebsocket::ixwebsocket
        nlohmann_json::nlohmann_json
        cppzmq
        bcrypt
)