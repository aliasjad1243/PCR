// CycleSequencer.h - runs a PCR temperature program:
//   initial denature -> N x (denature, anneal, extend) -> final extend.
// A step's hold timer only starts once the block is within `tolerance` of the
// step's setpoint, matching how the upstream firmware counts hold time.
#pragma once

#include <cstdint>

struct PcrStep {
    float celsius;
    uint32_t holdMs;
};

struct PcrProgram {
    PcrStep initial;   // e.g. 95 C, 120 s
    PcrStep denature;  // e.g. 95 C, 30 s
    PcrStep anneal;    // e.g. 60 C, 30 s
    PcrStep extend;    // e.g. 72 C, 30 s
    PcrStep final;     // e.g. 72 C, 300 s
    uint16_t cycles;   // e.g. 30
};

class CycleSequencer {
public:
    enum class Phase : uint8_t { Idle, Initial, Denature, Anneal, Extend, Final, Done };

    CycleSequencer(const PcrProgram& program, float tolerance);

    void start(uint32_t nowMs);
    // Feed the latest block temperature; advances the program when a hold
    // completes. Returns true when the setpoint changed this call.
    bool update(float blockCelsius, uint32_t nowMs);

    Phase phase() const { return phase_; }
    uint16_t cycle() const { return cycle_; }   // 1-based during cycling
    float setpoint() const;
    bool holding() const { return holding_; }

private:
    const PcrStep& step() const;
    void enter(Phase p, uint32_t nowMs);

    PcrProgram program_;
    float tolerance_;
    Phase phase_;
    uint16_t cycle_;
    bool holding_;
    uint32_t holdStartMs_;
};
