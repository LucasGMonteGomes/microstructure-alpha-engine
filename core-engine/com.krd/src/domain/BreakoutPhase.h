#pragma once

enum class BreakoutPhase {
    IDLE,
    BREAK_UP,
    BREAK_DOWN,
    HOLD_UP,
    HOLD_DOWN,
    CONFIRMED_UP,
    CONFIRMED_DOWN,
    FAILED
};