// Thermistor.h - NTC thermistor reading for the PocketPCR heater block and lid.
//
// Circuit (from the upstream PocketPCR schematic and firmware):
//
//   3.3 V ── R0 (4.7 kOhm) ──┬── ADC pin
//                            │
//                           NTC (10 kOhm @ 25 C, B = 3435)
//                            │
//                           GND
//
// The NTC is the lower leg of the divider, so
//   V_adc = 3.3 V * R_ntc / (R0 + R_ntc)   ->   R_ntc = R0 * V / (3.3 - V)
// and temperature follows from the Beta equation:
//   1/T = 1/T0 + ln(R_ntc / R_nominal) / B          (T in kelvin)
#pragma once

#include <cstdint>

class Thermistor {
public:
    struct Params {
        float supplyVolts;       // divider supply, 3.3 V
        float seriesOhms;        // R0, 4700 ohm
        float nominalOhms;       // NTC resistance at nominalKelvin, 10 kOhm
        float nominalKelvin;     // 298.15 K (25 C)
        float beta;              // 3435 K
        uint16_t adcFullScale;   // 4095 for a 12-bit ADC
    };

    static Params pocketPcrDefaults();

    explicit Thermistor(const Params& p);

    // Convert a raw ADC count to the NTC resistance in ohms.
    // Returns a negative value if the reading is at either rail (open/short).
    float resistanceFromAdc(uint16_t adc) const;

    // Convert a raw ADC count to degrees Celsius. Returns NAN on a rail reading.
    float celsiusFromAdc(uint16_t adc) const;

    // Inverse of the above, used by tests and for sanity checks:
    // expected ADC count for a given temperature.
    uint16_t adcFromCelsius(float celsius) const;

    // Exponential moving average with the same weighting as the upstream
    // firmware: mean = (3 * mean + new) / 4.
    float smooth(float newCelsius);
    void resetSmoothing(float celsius);

private:
    Params p_;
    float mean_;
    bool hasMean_;
};
