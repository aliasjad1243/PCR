// pcr_control.ino - runs a PCR program on the PocketPCR main board using the
// classes in this folder, and streams one CSV line per control period over USB:
//
//   ms,setpoint_c,block_c,effort,heater_duty,fan_duty,phase,cycle
//
// Those lines are what analysis/thermal_spc.py reads.
//
// Open this folder in the Arduino IDE, pick the SAMD21 (Arduino Zero) board, upload.
// Pin numbers match the upstream PocketPCR firmware (V1.02).

#include "CycleSequencer.h"
#include "HeaterDriver.h"
#include "PidController.h"
#include "Thermistor.h"

static const int kSensorPin = A2;
static const int kHeaterPin = 8;
static const int kFanPin = 4;
static const int kLidPin = 9;
static const uint32_t kPeriodMs = 100;

static Thermistor thermistor(Thermistor::pocketPcrDefaults());
static PidController pid({0.5f, 0.0001f, 0.15f}, 10.0f, 0.5f);  // output in C/s
static ArduinoPwmOutput* heaterOut;
static ArduinoPwmOutput* fanOut;
static HeaterDriver* driver;

// 95 C 120 s, then 30 x (95 C 30 s / 60 C 30 s / 72 C 30 s), then 72 C 300 s.
static const PcrProgram kProgram{{95.0f, 120000}, {95.0f, 30000}, {60.0f, 30000},
                                 {72.0f, 30000},  {72.0f, 300000}, 30};
static CycleSequencer sequencer(kProgram, 0.5f);

static uint32_t lastTick = 0;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    heaterOut = new ArduinoPwmOutput(kHeaterPin);
    fanOut = new ArduinoPwmOutput(kFanPin);
    driver = new HeaterDriver(*heaterOut, *fanOut, 255.0f, 255);
    pinMode(kLidPin, OUTPUT);
    analogWrite(kLidPin, 180);  // lid heater kept warm to stop condensation

    // Seed the filter so the first PID step is not computed from 0 C.
    driver->apply(0.0f);
    thermistor.resetSmoothing(thermistor.celsiusFromAdc(analogRead(kSensorPin)));

    Serial.println("ms,setpoint_c,block_c,effort,heater_duty,fan_duty,phase,cycle");
    sequencer.start(millis());
    lastTick = millis();
}

void loop() {
    const uint32_t now = millis();
    if (now - lastTick < kPeriodMs) return;
    const float dt = (now - lastTick) / 1000.0f;
    lastTick = now;

    // Read with both MOSFETs off so heater current does not shift the ADC ground.
    driver->apply(0.0f);
    const float raw = thermistor.celsiusFromAdc(analogRead(kSensorPin));
    // Open sensor reads far below 0 C, shorted reads far above 130 C.
    if (isnan(raw) || raw < -20.0f || raw > 130.0f) {  // fail safe
        driver->apply(0.0f);
        analogWrite(kLidPin, 0);
        Serial.println("# sensor fault, heaters off");
        while (true) {}
    }
    const float block = thermistor.smooth(raw);

    if (sequencer.update(block, now)) pid.reset();

    if (sequencer.phase() == CycleSequencer::Phase::Done) {
        analogWrite(kLidPin, 0);
        return;
    }

    const float sp = sequencer.setpoint();
    // PID output is a requested rate in C/s; the block model turns it into duty.
    const float effort = pid.update(sp, block, dt);
    driver->applyRate(effort, block);

    Serial.print(now); Serial.print(',');
    Serial.print(sp, 2); Serial.print(',');
    Serial.print(block, 2); Serial.print(',');
    Serial.print(effort, 2); Serial.print(',');
    Serial.print(driver->heaterDuty()); Serial.print(',');
    Serial.print(driver->fanDuty()); Serial.print(',');
    Serial.print(static_cast<int>(sequencer.phase())); Serial.print(',');
    Serial.println(sequencer.cycle());
}
