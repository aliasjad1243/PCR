// HeaterDriver.h - maps the PID effort onto the two low-side N-MOSFET switches
// on the PocketPCR main board: one drives the PCB-trace heater, one drives the
// cooling fan. Only one is ever on at a time.
//
// The hardware write is behind an interface so the logic can be unit tested
// on a PC; on the board, ArduinoPwmOutput (below) calls analogWrite().
#pragma once

#include <cstdint>

class PwmOutput {
public:
    virtual ~PwmOutput() = default;
    virtual void write(uint8_t duty) = 0;  // 0..255
};

class HeaterDriver {
public:
    // effortFullScale: PID effort that maps to 100 % duty.
    HeaterDriver(PwmOutput& heater, PwmOutput& fan, float effortFullScale, uint8_t maxDuty);

    // Linear mode: effort > 0 heats, effort < 0 cools, 0 turns both off.
    // Duty = |effort| / effortFullScale * maxDuty. Handy for open-loop bench tests.
    void apply(float effort);

    // Model mode (what the upstream PocketPCR firmware does): the PID output is
    // treated as a requested heating rate in C/s, and an empirical model of the
    // block (fitted by the upstream author) turns rate + block temperature into
    // a PWM duty. The model includes the duty needed just to hold temperature,
    // so the PID only has to correct the remainder.
    void applyRate(float rateCps, float blockC);

    static float heatingDutyFor(float rateCps, float blockC);
    static float coolingDutyFor(float rateCps, float blockC);

    uint8_t heaterDuty() const { return heaterDuty_; }
    uint8_t fanDuty() const { return fanDuty_; }

private:
    uint8_t toDuty(float magnitude) const;
    uint8_t clampDuty(float duty) const;
    void write();

    PwmOutput& heater_;
    PwmOutput& fan_;
    float effortFullScale_;
    uint8_t maxDuty_;
    uint8_t heaterDuty_;
    uint8_t fanDuty_;
};

#ifdef ARDUINO
#include <Arduino.h>
class ArduinoPwmOutput : public PwmOutput {
public:
    explicit ArduinoPwmOutput(int pin) : pin_(pin) { pinMode(pin_, OUTPUT); }
    void write(uint8_t duty) override { analogWrite(pin_, duty); }

private:
    int pin_;
};
#endif
