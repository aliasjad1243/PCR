#include "Thermistor.h"

#include <cmath>

Thermistor::Params Thermistor::pocketPcrDefaults() {
    return Params{3.3f, 4700.0f, 10000.0f, 298.15f, 3435.0f, 4095};
}

Thermistor::Thermistor(const Params& p) : p_(p), mean_(0.0f), hasMean_(false) {}

float Thermistor::resistanceFromAdc(uint16_t adc) const {
    if (adc == 0 || adc >= p_.adcFullScale) {
        return -1.0f;  // shorted or open sensor
    }
    const float volts = p_.supplyVolts * static_cast<float>(adc) / static_cast<float>(p_.adcFullScale);
    return p_.seriesOhms * volts / (p_.supplyVolts - volts);
}

float Thermistor::celsiusFromAdc(uint16_t adc) const {
    const float r = resistanceFromAdc(adc);
    if (r <= 0.0f) {
        return NAN;
    }
    const float invT = 1.0f / p_.nominalKelvin + std::log(r / p_.nominalOhms) / p_.beta;
    return 1.0f / invT - 273.15f;
}

uint16_t Thermistor::adcFromCelsius(float celsius) const {
    const float kelvin = celsius + 273.15f;
    const float r = p_.nominalOhms * std::exp(p_.beta * (1.0f / kelvin - 1.0f / p_.nominalKelvin));
    const float volts = p_.supplyVolts * r / (p_.seriesOhms + r);
    const float counts = volts / p_.supplyVolts * static_cast<float>(p_.adcFullScale);
    return static_cast<uint16_t>(std::lround(counts));
}

float Thermistor::smooth(float newCelsius) {
    if (!hasMean_) {
        resetSmoothing(newCelsius);
        return mean_;
    }
    mean_ = (3.0f * mean_ + newCelsius) / 4.0f;
    return mean_;
}

void Thermistor::resetSmoothing(float celsius) {
    mean_ = celsius;
    hasMean_ = true;
}
