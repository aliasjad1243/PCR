// Host-side unit tests for the PCR control classes.
// Build and run with:  make test   (from the firmware/ folder)
#include <cmath>
#include <cstdio>
#include <vector>

#include "../pcr_control/CycleSequencer.h"
#include "../pcr_control/HeaterDriver.h"
#include "../pcr_control/PidController.h"
#include "../pcr_control/Thermistor.h"

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            ++g_failures;                                                    \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                    \
    } while (0)

#define CHECK_NEAR(a, b, tol) CHECK(std::fabs((a) - (b)) <= (tol))

// ---------------------------------------------------------------- Thermistor
static void testThermistor() {
    std::printf("Thermistor\n");
    Thermistor t(Thermistor::pocketPcrDefaults());

    // At 25 C the NTC is 10 k, so V = 3.3 * 10k / 14.7k = 2.245 V -> 2786 counts.
    const uint16_t adc25 = t.adcFromCelsius(25.0f);
    CHECK(adc25 >= 2784 && adc25 <= 2788);
    CHECK_NEAR(t.resistanceFromAdc(adc25), 10000.0f, 15.0f);
    CHECK_NEAR(t.celsiusFromAdc(adc25), 25.0f, 0.05f);

    // Round trip across the PCR range: quantisation error stays under 0.1 C.
    for (float c : {4.0f, 37.0f, 55.0f, 60.0f, 72.0f, 95.0f, 98.0f}) {
        CHECK_NEAR(t.celsiusFromAdc(t.adcFromCelsius(c)), c, 0.1f);
    }

    // Hotter -> lower NTC resistance -> lower ADC count.
    CHECK(t.adcFromCelsius(95.0f) < t.adcFromCelsius(60.0f));

    // Rail readings are rejected.
    CHECK(std::isnan(t.celsiusFromAdc(0)));
    CHECK(std::isnan(t.celsiusFromAdc(4095)));
    // Near-rail readings fall outside the -20..130 C window the sketch uses
    // to detect an open (high count) or shorted (low count) sensor.
    CHECK(t.celsiusFromAdc(4094) < -20.0f);
    CHECK(t.celsiusFromAdc(1) > 130.0f);

    // Smoothing: (3 * old + new) / 4.
    t.resetSmoothing(60.0f);
    CHECK_NEAR(t.smooth(64.0f), 61.0f, 1e-4f);
    CHECK_NEAR(t.smooth(61.0f), 61.0f, 1e-4f);
}

// ---------------------------------------------------------------- PID
static void testPid() {
    std::printf("PidController\n");
    PidController pid({0.5f, 0.0001f, 0.15f}, 255.0f, 0.5f);

    // Far from setpoint: proportional only, integral held off.
    float out = pid.update(95.0f, 25.0f, 0.1f);
    CHECK_NEAR(out, 35.0f, 1e-3f);
    CHECK(!pid.integrating());

    // Output saturates at the limit.
    PidController hot({50.0f, 0.0f, 0.0f}, 255.0f, 0.5f);
    CHECK_NEAR(hot.update(95.0f, 25.0f, 0.1f), 255.0f, 1e-3f);
    CHECK_NEAR(hot.update(25.0f, 95.0f, 0.1f), -255.0f, 1e-3f);

    // Overshoot gives negative effort (fan).
    PidController cool({0.5f, 0.0001f, 0.15f}, 255.0f, 0.5f);
    CHECK(cool.update(60.0f, 70.0f, 0.1f) < 0.0f);

    // Inside the band the integrator turns on and accumulates.
    PidController near({0.5f, 0.1f, 0.0f}, 255.0f, 0.5f);
    near.update(60.0f, 59.8f, 1.0f);
    CHECK(near.integrating());
    near.update(60.0f, 59.8f, 1.0f);
    CHECK(near.integral() > 0.0f);

    // reset() clears state.
    near.reset();
    CHECK(!near.integrating());
    CHECK_NEAR(near.integral(), 0.0f, 1e-6f);
}

// ---------------------------------------------------------------- HeaterDriver
struct FakePwm : PwmOutput {
    std::vector<uint8_t> writes;
    void write(uint8_t d) override { writes.push_back(d); }
    uint8_t last() const { return writes.empty() ? 0 : writes.back(); }
};

static void testHeaterDriver() {
    std::printf("HeaterDriver\n");
    FakePwm heater, fan;
    HeaterDriver drv(heater, fan, 255.0f, 255);

    drv.apply(127.5f);
    CHECK(heater.last() == 128);
    CHECK(fan.last() == 0);

    drv.apply(-255.0f);
    CHECK(heater.last() == 0);
    CHECK(fan.last() == 255);

    drv.apply(1000.0f);  // clamps
    CHECK(heater.last() == 255);
    CHECK(fan.last() == 0);

    drv.apply(0.0f);
    CHECK(heater.last() == 0);
    CHECK(fan.last() == 0);

    // Heater and fan are never both non-zero after any apply().
    for (float e = -300.0f; e <= 300.0f; e += 7.5f) {
        drv.apply(e);
        CHECK(!(heater.last() > 0 && fan.last() > 0));
    }

    // Model mode: holding (rate ~ 0+) needs more duty at 95 C than at 25 C.
    const float hold25 = HeaterDriver::heatingDutyFor(0.0f, 25.0f);
    const float hold95 = HeaterDriver::heatingDutyFor(0.0f, 95.0f);
    CHECK(hold25 > 5.0f && hold25 < 15.0f);    // ~9.9
    CHECK(hold95 > 75.0f && hold95 < 90.0f);   // ~82.6
    // Asking for a faster ramp asks for more duty.
    CHECK(HeaterDriver::heatingDutyFor(1.0f, 60.0f) > HeaterDriver::heatingDutyFor(0.1f, 60.0f));
    // Fast cooling saturates the fan; gentle cooling does not.
    drv.applyRate(-1.0f, 60.0f);
    CHECK(fan.last() == 255 && heater.last() == 0);
    drv.applyRate(-0.2f, 60.0f);
    CHECK(fan.last() > 10 && fan.last() < 40);  // ~24
    drv.applyRate(0.5f, 72.0f);
    CHECK(heater.last() > 0 && fan.last() == 0);
}

// ---------------------------------------------------------------- Sequencer
static void testSequencer() {
    std::printf("CycleSequencer\n");
    PcrProgram prog{{95.0f, 2000}, {95.0f, 1000}, {60.0f, 1000}, {72.0f, 1000}, {72.0f, 3000}, 3};
    CycleSequencer seq(prog, 0.5f);

    CHECK(seq.phase() == CycleSequencer::Phase::Idle);
    CHECK(std::isnan(seq.setpoint()));

    uint32_t now = 0;
    seq.start(now);
    CHECK(seq.phase() == CycleSequencer::Phase::Initial);
    CHECK_NEAR(seq.setpoint(), 95.0f, 1e-6f);

    // Hold timer does not run while the block is still ramping.
    for (int i = 0; i < 50; ++i) {
        now += 100;
        seq.update(80.0f, now);
    }
    CHECK(seq.phase() == CycleSequencer::Phase::Initial);
    CHECK(!seq.holding());

    // Simple plant: block reaches the setpoint 300 ms after it changes.
    uint32_t changedAt = now;
    const uint32_t runStart = now;
    int denatureCount = 0;
    auto prev = seq.phase();
    while (seq.phase() != CycleSequencer::Phase::Done && now < 120000) {
        now += 50;
        float block = (now - changedAt >= 300) ? seq.setpoint() : 30.0f;
        if (seq.update(block, now)) {
            changedAt = now;
        }
        if (seq.phase() != prev) {
            if (seq.phase() == CycleSequencer::Phase::Denature) ++denatureCount;
            prev = seq.phase();
        }
    }
    CHECK(seq.phase() == CycleSequencer::Phase::Done);
    CHECK(denatureCount == 3);
    CHECK(seq.cycle() == 3);

    // Total time ~ initial 2 s + 3 x 3 s + final 3 s + ramps -> well under 20 s.
    const uint32_t elapsed = now - runStart;
    CHECK(elapsed > 14000 && elapsed < 20000);
}

int main() {
    testThermistor();
    testPid();
    testHeaterDriver();
    testSequencer();
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
