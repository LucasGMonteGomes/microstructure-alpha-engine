#include <iostream>
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>
#include <zmq.hpp>
#include <chrono>
#include <thread>
#include <string>
#include <mutex>
#include <iomanip>
#include <unordered_map>

#include "config/Config.h"
#include "domain/MarketSnapshot.h"
#include "domain/TradeTick.h"
#include "domain/FlowSnapshot.h"
#include "strategy/common/StrategyContext.h"
#include "execution/PaperTradeEngine.h"
#include "messaging/TradePublisher.h"
#include "flow/AggressionTracker.h"
#include "persistence/SignalPersistenceFilter.h"
#include "domain/RegimeSnapshot.h"
#include "regime/RegimeFilter.h"
#include "execution/TradeStatsCollector.h"
#include "execution/TradeCsvWriter.h"
#include "execution/TestSummaryWriter.h"
#include "execution/TestRunPaths.h"
#include "execution/ExecutionQualityFilter.h"
#include "strategy/breakout_confirmation_v2/BreakoutConfirmationV2Strategy.h"
#include "domain/PriceStructureSnapshot.h"
#include "domain/BreakoutState.h"
#include "domain/BreakoutPhase.h"
#include "structure/BreakoutStateEngine.h"
#include "structure/PriceStructureEngine.h"
#include "execution/SignalDiagnosticsCollector.h"
#include "execution/SignalDiagnosticsWriter.h"

using json = nlohmann::json;

std::mutex zmq_mutex;
std::mutex strategy_mutex;

namespace {
    std::int64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    std::string makeKey(const std::string &exchange, const std::string &symbol) {
        return exchange + "|" + symbol;
    }

    double calculateMidPrice(double bidPrice, double askPrice) {
        return (bidPrice + askPrice) / 2.0;
    }

    double calculateSpread(double bidPrice, double askPrice) {
        return askPrice - bidPrice;
    }

    double calculateSpreadBps(double bidPrice, double askPrice) {
        const double midPrice = calculateMidPrice(bidPrice, askPrice);
        if (midPrice <= 0.0) {
            return 0.0;
        }

        return ((askPrice - bidPrice) / midPrice) * 10000.0;
    }

    double calculateImbalancePct(double bidQty, double askQty) {
        const double totalQty = bidQty + askQty;
        return (totalQty > 0.0) ? (bidQty / totalQty) * 100.0 : 50.0;
    }

    double calculateRecentMoveBps(double currentMidPrice, double previousMidPrice) {
        if (previousMidPrice <= 0.0) {
            return 0.0;
        }

        return ((currentMidPrice - previousMidPrice) / previousMidPrice) * 10000.0;
    }

    MarketSnapshot buildSnapshot(const std::string &exchange,
                                 const std::string &symbol,
                                 double bidPrice,
                                 double askPrice,
                                 double bidQty,
                                 double askQty) {
        MarketSnapshot snapshot;
        snapshot.exchange = exchange;
        snapshot.symbol = symbol;
        snapshot.bidPrice = bidPrice;
        snapshot.askPrice = askPrice;
        snapshot.bidQty = bidQty;
        snapshot.askQty = askQty;
        snapshot.midPrice = calculateMidPrice(bidPrice, askPrice);
        snapshot.spread = calculateSpread(bidPrice, askPrice);
        snapshot.spreadBps = calculateSpreadBps(bidPrice, askPrice);
        snapshot.imbalance = calculateImbalancePct(bidQty, askQty);
        snapshot.timestampMs = nowMs();
        return snapshot;
    }

    std::string signalSideToString(SignalSide side) {
        switch (side) {
            case SignalSide::LONG:
                return "LONG";
            case SignalSide::SHORT:
                return "SHORT";
            case SignalSide::HOLD:
            default:
                return "HOLD";
        }
    }

    std::string exitReasonToString(ExitReason reason) {
        switch (reason) {
            case ExitReason::TAKE_PROFIT:
                return "TAKE_PROFIT";
            case ExitReason::STOP_LOSS:
                return "STOP_LOSS";
            case ExitReason::TIMEOUT:
                return "TIMEOUT";
            case ExitReason::INVALIDATION:
                return "INVALIDATION";
            case ExitReason::NONE:
            default:
                return "NONE";
        }
    }

    std::string breakoutPhaseToString(BreakoutPhase phase) {
        switch (phase) {
            case BreakoutPhase::IDLE:
                return "IDLE";
            case BreakoutPhase::BREAK_UP:
                return "BREAK_UP";
            case BreakoutPhase::BREAK_DOWN:
                return "BREAK_DOWN";
            case BreakoutPhase::HOLD_UP:
                return "HOLD_UP";
            case BreakoutPhase::HOLD_DOWN:
                return "HOLD_DOWN";
            case BreakoutPhase::CONFIRMED_UP:
                return "CONFIRMED_UP";
            case BreakoutPhase::CONFIRMED_DOWN:
                return "CONFIRMED_DOWN";
            case BreakoutPhase::FAILED:
                return "FAILED";
            default:
                return "UNKNOWN";
        }
    }

    AggressorSide parseBinanceAggressor(bool isBuyerMaker) {
        // Se o comprador foi maker, então o agressor foi o vendedor.
        return isBuyerMaker ? AggressorSide::SELL : AggressorSide::BUY;
    }

    AggressorSide parseBybitAggressor(const std::string &side) {
        // Bybit reporta "Buy" ou "Sell" do lado agressor.
        if (side == "Buy") {
            return AggressorSide::BUY;
        }

        if (side == "Sell") {
            return AggressorSide::SELL;
        }

        return AggressorSide::UNKNOWN;
    }
}

void processSnapshot(
    const MarketSnapshot &snapshot,
    BreakoutConfirmationV2Strategy &strategy,
    ExecutionQualityFilter &executionQualityFilter,
    PaperTradeEngine &paperTradeEngine,
    TradePublisher &tradePublisher,
    TradeStatsCollector &tradeStatsCollector,
    TradeCsvWriter &tradeCsvWriter,
    SignalDiagnosticsCollector &signalDiagnosticsCollector,
    AggressionTracker &aggressionTracker,
    SignalPersistenceFilter &persistenceFilter,
    RegimeFilter &regimeFilter,
    PriceStructureEngine &priceStructureEngine,
    BreakoutStateEngine &breakoutStateEngine,
    std::unordered_map<std::string, double> &lastMidPriceByKey,
    std::unordered_map<std::string, MarketSnapshot> &lastSnapshotByKey
) {
    std::lock_guard<std::mutex> engineLock(strategy_mutex);

    const std::string key = makeKey(snapshot.exchange, snapshot.symbol);
    lastSnapshotByKey[key] = snapshot;

    double previousMidPrice = 0.0;
    const auto it = lastMidPriceByKey.find(key);
    if (it != lastMidPriceByKey.end()) {
        previousMidPrice = it->second;
    }

    const double recentMoveBps = calculateRecentMoveBps(snapshot.midPrice, previousMidPrice);
    lastMidPriceByKey[key] = snapshot.midPrice;

    FlowSnapshot flowSnapshot = aggressionTracker.getSnapshot(
        snapshot.exchange,
        snapshot.symbol,
        snapshot.timestampMs
    );

    regimeFilter.onSnapshot(snapshot);

    RegimeSnapshot regimeSnapshot = regimeFilter.getSnapshot(
        snapshot.exchange,
        snapshot.symbol,
        snapshot.timestampMs,
        flowSnapshot
    );

    PriceStructureSnapshot priceStructureSnapshot = priceStructureEngine.update(snapshot);
    BreakoutState breakoutState = breakoutStateEngine.update(snapshot, priceStructureSnapshot);

    StrategyContext context;
    context.marketSnapshot = snapshot;
    context.flowSnapshot = flowSnapshot;
    context.regimeSnapshot = regimeSnapshot;
    context.priceStructureSnapshot = priceStructureSnapshot;
    context.breakoutState = breakoutState;
    context.recentMoveBps = recentMoveBps;

    SignalResult signal = strategy.evaluate(context);

    auto tradeResultOpt = paperTradeEngine.update(snapshot, signal);
    if (tradeResultOpt.has_value()) {
        const auto &trade = tradeResultOpt.value();

        {
            std::lock_guard<std::mutex> zmqLock(zmq_mutex);
            tradePublisher.publish(trade);
        }

        tradeStatsCollector.onTradeClosed(trade);
        tradeCsvWriter.writeTrade(trade);

        std::cout << std::fixed << std::setprecision(6)
                  << "[TRADE CLOSED] "
                  << trade.exchange << " "
                  << trade.symbol << " "
                  << signalSideToString(trade.side)
                  << " entry=" << trade.entryPrice
                  << " exit=" << trade.exitPrice
                  << " reason=" << exitReasonToString(trade.exitReason)
                  << " gross=" << trade.grossPnlPct << "%"
                  << " fee=" << trade.feePct << "%"
                  << " slippage=" << trade.slippagePct << "%"
                  << " net=" << trade.netPnlPct << "%"
                  << std::endl;
    }

    bool persistenceApproved = false;
    if (regimeSnapshot.tradable) {
        persistenceApproved = persistenceFilter.shouldAllowEntry(snapshot, signal);
    }

    bool executionApproved = false;
    if (signal.isValid) {
        executionApproved = executionQualityFilter.shouldAllowEntry(snapshot, signal);
    }

    signalDiagnosticsCollector.onSignal(snapshot,
                                        signal,
                                        regimeSnapshot,
                                        priceStructureSnapshot,
                                        breakoutState,
                                        persistenceApproved,
                                        executionApproved);

    std::cout << std::fixed << std::setprecision(2)
              << "[SIGNAL] "
              << snapshot.exchange << " "
              << snapshot.symbol
              << " bid=" << snapshot.bidPrice
              << " ask=" << snapshot.askPrice
              << " imbalance=" << snapshot.imbalance
              << " spreadBps=" << snapshot.spreadBps
              << " recentMoveBps=" << recentMoveBps
              << " flowBias=" << signal.flowBias
              << " flowStrength=" << signal.flowStrength
              << " regimeAvgSpread=" << regimeSnapshot.avgSpreadBps
              << " regimeRangeBps=" << regimeSnapshot.shortRangeBps
              << " regimeActivityBps=" << regimeSnapshot.activityBps
              << " regimeImbalanceRange=" << regimeSnapshot.imbalanceRange
              << " regimeHasRecentFlow=" << (regimeSnapshot.hasRecentFlow ? "true" : "false")
              << " regimeTradable=" << (regimeSnapshot.tradable ? "true" : "false")
              << " regimeSamples=" << regimeSnapshot.sampleCount
              << " regimeReason=" << regimeSnapshot.reason
              << " side=" << signalSideToString(signal.side)
              << " confidence=" << signal.confidence
              << " expectedMoveBps=" << signal.expectedMoveBps
              << " valid=" << (signal.isValid ? "true" : "false")
              << " persistenceApproved=" << (persistenceApproved ? "true" : "false")
              << " executionApproved=" << (executionApproved ? "true" : "false")
              << " reason=" << signal.reason
              << " structureRangeBps=" << priceStructureSnapshot.rangeBps
              << " breakoutDistanceBps=" << priceStructureSnapshot.breakoutDistanceBps
              << " isBreakoutUp=" << (priceStructureSnapshot.isBreakoutUp ? "true" : "false")
              << " isBreakoutDown=" << (priceStructureSnapshot.isBreakoutDown ? "true" : "false")
              << " breakoutActive=" << (breakoutState.active ? "true" : "false")
              << " breakoutReturnedInside=" << (breakoutState.returnedInsideRange ? "true" : "false")
              << " breakoutPhase=" << breakoutPhaseToString(breakoutState.phase)
              << " entryConsumed=" << (breakoutState.entryConsumed ? "true" : "false")
              << std::endl;

    if (regimeSnapshot.tradable &&
        persistenceApproved &&
        executionApproved &&
        paperTradeEngine.tryOpenPosition(snapshot, signal)) {

        breakoutStateEngine.markEntryConsumed(snapshot.exchange, snapshot.symbol);

        std::cout << "[BREAKOUT CONSUMED] "
                  << snapshot.exchange << " "
                  << snapshot.symbol
                  << std::endl;

        const Position &pos = paperTradeEngine.getOpenPosition();

        std::cout << std::fixed << std::setprecision(6)
                  << "[TRADE OPEN] "
                  << pos.exchange << " "
                  << pos.symbol << " "
                  << signalSideToString(pos.side)
                  << " entry=" << pos.entryPrice
                  << " target=" << pos.targetPrice
                  << " stop=" << pos.stopPrice
                  << " timeoutMs=" << pos.timeoutTimestampMs
                  << std::endl;
    }
}

int main() {
    Config config;
    BreakoutConfirmationV2Strategy strategy(config);
    ExecutionQualityFilter executionQualityFilter(config);
    PaperTradeEngine paperTradeEngine(config);
    PriceStructureEngine priceStructureEngine(config);
    BreakoutStateEngine breakoutStateEngine;

    // Configuração de tempo de duração do teste.
    // const long testDurationMs = 5L * 60L * 1000L; // 05 minutos
    // const long testDurationMs = 10L * 60L * 1000L; // 10 minutos
    const long testDurationMs = 15L * 60L * 1000L; // 15 minutos
    // const long testDurationMs = 20L * 60L * 1000L; // 20 minutos
    // const long testDurationMs = 30L * 60L * 1000L; // 30 minutos
    // const long testDurationMs = 60L * 60L * 1000L; // 01 hora
    // const long testDurationMs = 3L * 60L * 60L * 1000L; // 03 horas

    const std::int64_t testStartMs = nowMs();
    const std::int64_t testEndMs = testStartMs + testDurationMs;

    const TestRunPaths testRunPaths = TestRunPathsBuilder::build(testDurationMs);

    TradeStatsCollector tradeStatsCollector;
    TradeCsvWriter tradeCsvWriter(testRunPaths.tradeResultsCsvPath);
    SignalDiagnosticsCollector signalDiagnosticsCollector;

    // Janela de 5 segundos para fluxo agressor.
    AggressionTracker aggressionTracker(5000);

    SignalPersistenceFilter persistenceFilter(0, 1);
    RegimeFilter regimeFilter(3000);

    zmq::context_t context(1);
    zmq::socket_t publisher(context, zmq::socket_type::pub);
    publisher.bind("tcp://*:5555");

    TradePublisher tradePublisher(publisher);

    std::unordered_map<std::string, double> lastMidPriceByKey;
    std::unordered_map<std::string, MarketSnapshot> lastSnapshotByKey;

    ix::initNetSystem();

    // ==========================================
    // 1. BINANCE
    // ==========================================
    ix::WebSocket binance_ws;
    binance_ws.setUrl(
        "wss://stream.binance.com:9443/stream?streams="
        "btcusdt@bookTicker/"
        "ethusdt@bookTicker/"
        "btcusdt@aggTrade/"
        "ethusdt@aggTrade"
    );

    binance_ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << "[REDE] Binance conectada." << std::endl;
            return;
        }

        if (msg->type != ix::WebSocketMessageType::Message) {
            return;
        }

        try {
            auto j = json::parse(msg->str);
            if (!j.contains("data")) {
                return;
            }

            const auto &data = j["data"];

            // BookTicker
            if (data.contains("s") &&
                data.contains("b") &&
                data.contains("a") &&
                data.contains("B") &&
                data.contains("A")) {

                const std::string symbol = data["s"];
                const double bidPrice = std::stod(std::string(data["b"]));
                const double askPrice = std::stod(std::string(data["a"]));
                const double bidQty = std::stod(std::string(data["B"]));
                const double askQty = std::stod(std::string(data["A"]));

                MarketSnapshot snapshot = buildSnapshot(
                    "BINANCE",
                    symbol,
                    bidPrice,
                    askPrice,
                    bidQty,
                    askQty
                );

                processSnapshot(snapshot,
                                strategy,
                                executionQualityFilter,
                                paperTradeEngine,
                                tradePublisher,
                                tradeStatsCollector,
                                tradeCsvWriter,
                                signalDiagnosticsCollector,
                                aggressionTracker,
                                persistenceFilter,
                                regimeFilter,
                                priceStructureEngine,
                                breakoutStateEngine,
                                lastMidPriceByKey,
                                lastSnapshotByKey);
                return;
            }

            // aggTrade
            if (data.contains("s") &&
                data.contains("p") &&
                data.contains("q") &&
                data.contains("m")) {

                TradeTick trade;
                trade.exchange = "BINANCE";
                trade.symbol = data["s"];
                trade.price = std::stod(std::string(data["p"]));
                trade.qty = std::stod(std::string(data["q"]));
                trade.aggressorSide = parseBinanceAggressor(static_cast<bool>(data["m"]));
                trade.timestampMs = nowMs();

                {
                    std::lock_guard<std::mutex> engineLock(strategy_mutex);
                    aggressionTracker.onTrade(trade);
                }

                return;
            }
        } catch (const std::exception &e) {
            std::cerr << "[ERRO][BINANCE] " << e.what() << std::endl;
        }
    });

    // ==========================================
    // 2. BYBIT
    // ==========================================
    ix::WebSocket bybit_ws;
    bybit_ws.setUrl("wss://stream.bybit.com/v5/public/spot");

    bybit_ws.setOnMessageCallback([&](const ix::WebSocketMessagePtr &msg) {
        if (msg->type == ix::WebSocketMessageType::Open) {
            std::cout << "[REDE] Bybit conectada. Assinando orderbook e trades..." << std::endl;

            const std::string sub_msg =
                "{\"op\":\"subscribe\",\"args\":["
                "\"orderbook.1.BTCUSDT\","
                "\"orderbook.1.ETHUSDT\","
                "\"publicTrade.BTCUSDT\","
                "\"publicTrade.ETHUSDT\""
                "]}";

            bybit_ws.sendText(sub_msg);
            return;
        }

        if (msg->type != ix::WebSocketMessageType::Message) {
            return;
        }

        try {
            auto j = json::parse(msg->str);

            if (!j.contains("topic") || !j.contains("data")) {
                return;
            }

            const std::string topic = j["topic"];

            // orderbook
            if (topic.find("orderbook.") == 0) {
                if (!j["data"].contains("b") || !j["data"].contains("a")) {
                    return;
                }

                if (j["data"]["b"].empty() || j["data"]["a"].empty()) {
                    return;
                }

                const std::string symbol = j["data"]["s"];
                const double bidPrice = std::stod(std::string(j["data"]["b"][0][0]));
                const double askPrice = std::stod(std::string(j["data"]["a"][0][0]));
                const double bidQty = std::stod(std::string(j["data"]["b"][0][1]));
                const double askQty = std::stod(std::string(j["data"]["a"][0][1]));

                MarketSnapshot snapshot = buildSnapshot(
                    "BYBIT",
                    symbol,
                    bidPrice,
                    askPrice,
                    bidQty,
                    askQty
                );

                processSnapshot(snapshot,
                                strategy,
                                executionQualityFilter,
                                paperTradeEngine,
                                tradePublisher,
                                tradeStatsCollector,
                                tradeCsvWriter,
                                signalDiagnosticsCollector,
                                aggressionTracker,
                                persistenceFilter,
                                regimeFilter,
                                priceStructureEngine,
                                breakoutStateEngine,
                                lastMidPriceByKey,
                                lastSnapshotByKey);
                return;
            }

            // publicTrade
            if (topic.find("publicTrade.") == 0) {
                for (const auto &item : j["data"]) {
                    if (!item.contains("s") ||
                        !item.contains("p") ||
                        !item.contains("v") ||
                        !item.contains("S")) {
                        continue;
                    }

                    TradeTick trade;
                    trade.exchange = "BYBIT";
                    trade.symbol = item["s"];
                    trade.price = std::stod(std::string(item["p"]));
                    trade.qty = std::stod(std::string(item["v"]));
                    trade.aggressorSide = parseBybitAggressor(std::string(item["S"]));
                    trade.timestampMs = nowMs();

                    {
                        std::lock_guard<std::mutex> engineLock(strategy_mutex);
                        aggressionTracker.onTrade(trade);
                    }
                }

                return;
            }
        } catch (const std::exception &e) {
            std::cerr << "[ERRO][BYBIT] " << e.what() << std::endl;
        }
    });

    std::cout << "[SYSTEM] Core Engine v3-lite + Flow iniciado..." << std::endl;

    binance_ws.start();
    bybit_ws.start();

    while (nowMs() < testEndMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    binance_ws.stop();
    bybit_ws.stop();

    {
        std::lock_guard<std::mutex> engineLock(strategy_mutex);
        if (paperTradeEngine.hasOpenPosition()) {
            const Position& openPosition = paperTradeEngine.getOpenPosition();
            const std::string openKey = makeKey(openPosition.exchange, openPosition.symbol);

            auto it = lastSnapshotByKey.find(openKey);
            if (it != lastSnapshotByKey.end()) {
                auto forcedCloseOpt = paperTradeEngine.forceClosePosition(it->second, ExitReason::TIMEOUT);

                if (forcedCloseOpt.has_value()) {
                    const auto& trade = forcedCloseOpt.value();

                    {
                        std::lock_guard<std::mutex> zmqLock(zmq_mutex);
                        tradePublisher.publish(trade);
                    }

                    tradeStatsCollector.onTradeClosed(trade);
                    tradeCsvWriter.writeTrade(trade);

                    std::cout << std::fixed << std::setprecision(6)
                              << "[TRADE FORCE CLOSED AT TEST END] "
                              << trade.exchange << " "
                              << trade.symbol << " "
                              << signalSideToString(trade.side)
                              << " entry=" << trade.entryPrice
                              << " exit=" << trade.exitPrice
                              << " reason=" << exitReasonToString(trade.exitReason)
                              << " gross=" << trade.grossPnlPct << "%"
                              << " fee=" << trade.feePct << "%"
                              << " slippage=" << trade.slippagePct << "%"
                              << " net=" << trade.netPnlPct << "%"
                              << std::endl;
                }
            }
        }
    }
    const bool summaryWritten =
        TestSummaryWriter::writeSummary(testRunPaths.testSummaryCsvPath,
                                        tradeStatsCollector,
                                        testDurationMs,
                                        testStartMs,
                                        testEndMs);

    const bool diagnosticSummaryWritten =
        SignalDiagnosticsWriter::writeSummary(testRunPaths.diagnosticSummaryCsvPath,
                                              signalDiagnosticsCollector);

    std::cout << "\n[TEST OUTPUT]\n"
              << "runDirectory=" << testRunPaths.runDirectory << "\n"
              << "tradeResultsCsvPath=" << testRunPaths.tradeResultsCsvPath << "\n"
              << "testSummaryCsvPath=" << testRunPaths.testSummaryCsvPath << "\n"
              << "diagnosticSummaryCsvPath=" << testRunPaths.diagnosticSummaryCsvPath << "\n";

    std::cout << "\n[TEST SUMMARY]\n"
              << "durationMs=" << testDurationMs << "\n"
              << "totalTrades=" << tradeStatsCollector.getTotalTrades() << "\n"
              << "takeProfitCount=" << tradeStatsCollector.getTakeProfitCount() << "\n"
              << "stopLossCount=" << tradeStatsCollector.getStopLossCount() << "\n"
              << "timeoutCount=" << tradeStatsCollector.getTimeoutCount() << "\n"
              << "winCount=" << tradeStatsCollector.getWinCount() << "\n"
              << "lossCount=" << tradeStatsCollector.getLossCount() << "\n"
              << "grossPnlSumPct=" << tradeStatsCollector.getGrossPnlSumPct() << "\n"
              << "netPnlSumPct=" << tradeStatsCollector.getNetPnlSumPct() << "\n"
              << "winRatePct=" << tradeStatsCollector.getWinRatePct() << "\n"
              << "averageNetPnlPct=" << tradeStatsCollector.getAverageNetPnlPct() << "\n"
              << "tradeCsvOpen=" << (tradeCsvWriter.isOpen() ? "true" : "false") << "\n"
              << "summaryWritten=" << (summaryWritten ? "true" : "false") << "\n"
              << "diagnosticSummaryWritten=" << (diagnosticSummaryWritten ? "true" : "false") << "\n"
              << std::endl;

    signalDiagnosticsCollector.printSummary();

    ix::uninitNetSystem();

    return 0;
}