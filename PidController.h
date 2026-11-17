// PidController.h - temperature PID with the integral term held off until the
// block is close to the setpoint (same strategy as the upstream PocketPCR code,
// which only enables integration once |error| < tolerance).
//
// Output is a signed "effort": positive means heat, negative means cool (fan).
#pragma once

class PidController {
public:
    struct Gains {
        float kp;
        float ki;
        float kd;
    };

    PidController(const Gains& g, float outputLimit, float integralBand);

    // Call once per control period.
    // setpoint, measured: degrees C. dtSeconds: time since the previous call.
    float update(float setpoint, float measured, float dtSeconds);

    // Clear integral and derivative history (call when the setpoint changes).
    void reset();

    bool integrating() const { return integrating_; }
    float integral() const { return integral_; }
    void setGains(const Gains& g) { g_ = g; }
    const Gains& gains() const { return g_; }

private:
    Gains g_;
    float outputLimit_;
    float integralBand_;
    float integral_;
    float lastError_;
    bool hasLast_;
    bool integrating_;
};
