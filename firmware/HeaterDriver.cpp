#include "HeaterDriver.h"

#include <cmath>

HeaterDriver::HeaterDriver(PwmOutput& heater, PwmOutput& fan, float effortFullScale, uint8_t maxDuty)
    : heater_(heater),
      fan_(fan),
      effortFullScale_(effortFullScale),
      maxDuty_(maxDuty),
      heaterDuty_(0),
      fanDuty_(0) {}

uint8_t HeaterDriver::toDuty(float magnitude) const {
    float scaled = magnitude / effortFullScale_ * static_cast<float>(maxDuty_);
    if (scaled < 0.0f) scaled = 0.0f;
    if (scaled > static_cast<float>(maxDuty_)) scaled = static_cast<float>(maxDuty_);
    return static_cast<uint8_t>(std::lround(scaled));
}

void HeaterDriver::apply(float effort) {
    if (effort > 0.0f) {
        heaterDuty_ = toDuty(effort);
        fanDuty_ = 0;
    } else if (effort < 0.0f) {
        heaterDuty_ = 0;
        fanDuty_ = toDuty(-effort);
    } else {
        heaterDuty_ = 0;
        fanDuty_ = 0;
    }
    write();
}

uint8_t HeaterDriver::clampDuty(float duty) const {
    if (duty < 0.0f) duty = 0.0f;
    if (duty > static_cast<float>(maxDuty_)) duty = static_cast<float>(maxDuty_);
    return static_cast<uint8_t>(std::lround(duty));
}

// Coefficients copied from power_heating() / power_cooling() in the upstream
// PocketPCR firmware V1.02 (GPL-3.0, Urs Gaudenz).
float HeaterDriver::heatingDutyFor(float rateCps, float blockC) {
    const float c = 0.00564542f, d = 0.0418254f, e = 0.000019826f, f = 0.003711f;
    return (d - f * blockC - rateCps) / (e * blockC - c);
}

float HeaterDriver::coolingDutyFor(float rateCps, float blockC) {
    const float i = 0.000072781f, j = 0.00413579f, k = 0.001372876f, l = 0.1204656f;
    return (l - j * blockC - rateCps) / (i * blockC - k);
}

void HeaterDriver::applyRate(float rateCps, float blockC) {
    if (rateCps > 0.0f) {
        heaterDuty_ = clampDuty(heatingDutyFor(rateCps, blockC));
        fanDuty_ = 0;
    } else if (rateCps < 0.0f) {
        heaterDuty_ = 0;
        fanDuty_ = clampDuty(coolingDutyFor(rateCps, blockC));
    } else {
        heaterDuty_ = 0;
        fanDuty_ = 0;
    }
    write();
}

void HeaterDriver::write() {
    // Turn the opposing output off first so heater and fan never overlap.
    if (heaterDuty_ > 0) {
        fan_.write(0);
        heater_.write(heaterDuty_);
    } else {
        heater_.write(0);
        fan_.write(fanDuty_);
    }
}
