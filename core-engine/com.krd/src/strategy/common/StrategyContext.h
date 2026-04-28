#pragma once

#include "../../domain/MarketSnapshot.h"
#include "../../domain/FlowSnapshot.h"
#include "../../domain/RegimeSnapshot.h"
#include "../../domain/PriceStructureSnapshot.h"
#include "../../domain/BreakoutState.h"

struct StrategyContext {
    MarketSnapshot marketSnapshot;
    FlowSnapshot flowSnapshot;
    RegimeSnapshot regimeSnapshot;
    PriceStructureSnapshot priceStructureSnapshot;
    BreakoutState breakoutState;
    double recentMoveBps{0.0};
};