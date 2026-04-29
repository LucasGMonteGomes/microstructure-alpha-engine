#pragma once

struct Config {
    double imbalanceLongThreshold{58.0};
    double imbalanceShortThreshold{42.0};

    double minConfidence{0.45};

    double maxSpreadBps{6.0};
    double maxRecentMoveBps{80.0};
    double minExpectedMoveBps{25.0};

    double minFlowStrength{0.02};
    double minFlowBiasAbs{0.68};

    double targetPct{0.70};
    double stopPct{0.25};
    long timeoutMs{15000};

    double feePctPerSide{0.01};
    double slippagePctPerSide{0.0025};
};