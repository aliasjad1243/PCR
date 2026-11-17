#include "PidController.h"

#include <cmath>

PidController::PidController(const Gains& g, float outputLimit, float integralBand)
    : g_(g),
      outputLimit_(outputLimit),
      integralBand_(integralBand),
      integral_(0.0f),
      lastError_(0.0f),
      hasLast_(false),
      integrating_(false) {}

void PidController::reset() {
    integral_ = 0.0f;
    lastError_ = 0.0f;
    hasLast_ = false;
    integrating_ = false;
}

float PidController::update(float setpoint, float measured, float dtSeconds) {
    const float error = setpoint - measured;

    // Hold the integrator off during large transitions to avoid wind-up;
    // enable it once the block is inside the band around the setpoint.
    if (!integrating_ && std::fabs(error) < integralBand_) {
        integrating_ = true;
        integral_ = 0.0f;
    }

    float derivative = 0.0f;
    if (hasLast_ && dtSeconds > 0.0f) {
        derivative = (error - lastError_) / dtSeconds;
    }
    lastError_ = error;
    hasLast_ = true;

    float output = g_.kp * error + g_.kd * derivative;

    if (integrating_ && dtSeconds > 0.0f) {
        const float candidate = integral_ + error * dtSeconds;
        const float withI = output + g_.ki * candidate;
        // Anti-windup: only accept the new integral if it does not push the
        // output further into saturation.
        if (std::fabs(withI) <= outputLimit_ || (withI > 0.0f) != (error > 0.0f)) {
            integral_ = candidate;
        }
        output += g_.ki * integral_;
    }

    if (output > outputLimit_) output = outputLimit_;
    if (output < -outputLimit_) output = -outputLimit_;
    return output;
}
