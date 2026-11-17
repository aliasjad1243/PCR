#include "CycleSequencer.h"

#include <cmath>

CycleSequencer::CycleSequencer(const PcrProgram& program, float tolerance)
    : program_(program),
      tolerance_(tolerance),
      phase_(Phase::Idle),
      cycle_(0),
      holding_(false),
      holdStartMs_(0) {}

const PcrStep& CycleSequencer::step() const {
    switch (phase_) {
        case Phase::Initial: return program_.initial;
        case Phase::Denature: return program_.denature;
        case Phase::Anneal: return program_.anneal;
        case Phase::Extend: return program_.extend;
        case Phase::Final: return program_.final;
        default: return program_.final;
    }
}

float CycleSequencer::setpoint() const {
    if (phase_ == Phase::Idle || phase_ == Phase::Done) {
        return NAN;
    }
    return step().celsius;
}

void CycleSequencer::enter(Phase p, uint32_t nowMs) {
    phase_ = p;
    holding_ = false;
    holdStartMs_ = nowMs;
}

void CycleSequencer::start(uint32_t nowMs) {
    cycle_ = 0;
    enter(Phase::Initial, nowMs);
}

bool CycleSequencer::update(float blockCelsius, uint32_t nowMs) {
    if (phase_ == Phase::Idle || phase_ == Phase::Done) {
        return false;
    }

    if (!holding_) {
        if (std::fabs(blockCelsius - step().celsius) < tolerance_) {
            holding_ = true;
            holdStartMs_ = nowMs;
        }
        return false;
    }

    if (nowMs - holdStartMs_ < step().holdMs) {
        return false;
    }

    // Hold complete: advance.
    switch (phase_) {
        case Phase::Initial:
            cycle_ = 1;
            enter(Phase::Denature, nowMs);
            break;
        case Phase::Denature:
            enter(Phase::Anneal, nowMs);
            break;
        case Phase::Anneal:
            enter(Phase::Extend, nowMs);
            break;
        case Phase::Extend:
            if (cycle_ < program_.cycles) {
                ++cycle_;
                enter(Phase::Denature, nowMs);
            } else {
                enter(Phase::Final, nowMs);
            }
            break;
        case Phase::Final:
            phase_ = Phase::Done;
            holding_ = false;
            break;
        default:
            break;
    }
    return true;
}
