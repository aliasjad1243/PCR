# Portable PCR Machine

My contributions to **[PocketPCR](https://github.com/GaudiLabs/PocketPCR)**, an open-source,
USB-powered thermal cycler by Urs Gaudenz (GaudiLabs). A PCR machine copies DNA by cycling a
small heated block between about 95 °C (denature), 60 °C (anneal) and 72 °C (extend), 30 or
more times, so everything comes down to how well the block hits and holds temperature.

I built the machine from the PocketPCR design, reworked the heater driver, restructured the
control firmware into tested C++ classes, and tuned the temperature control.
**This repo only holds the files I wrote or changed.** The KiCad boards, Gerbers and original
firmware are in the PocketPCR repo.

## The original project

| | |
|---|---|
| Repo | [GaudiLabs/PocketPCR](https://github.com/GaudiLabs/PocketPCR) (GPL-3.0) · [gaudi.ch/PocketPCR](http://gaudi.ch/PocketPCR/) |
| Boards | main board (SAMD21, USB-C, OLED, rotary encoder), PCB-trace bottom heater with NTC, heated lid |
| Firmware | single Arduino sketch, V1.02 |

## My contributions (what I did → where to look)

1. **Heater driver (KiCad + LTspice):** logic-level MOSFETs, gate pull-downs so the heater stays
   off while the chip boots, and a flyback diode for the fan; simulated the low-side heater switch
   in LTspice → [`hardware/heater_driver_notes.md`](hardware/heater_driver_notes.md),
   [`hardware/heater_driver.cir`](hardware/heater_driver.cir)
2. **Firmware refactor (object-oriented C++, unit tests, Git):** split the single sketch into
   `Thermistor` (ADC → °C), `PidController`, `HeaterDriver` (heater/fan PWM, never both on) and
   `CycleSequencer` (95 → 60 → 72 °C program), with host-side unit tests → [`firmware/`](firmware/)
3. **PID heater + PWM fan tuning, checked against a reference thermocouple:** the sketch logs one
   CSV line every 100 ms; the analysis script reports overshoot, settling and hold stability, and
   compares the block to a thermocouple at 95 / 60 / 72 °C (target ±1 °C)
   → [`analysis/thermal_spc.py`](analysis/thermal_spc.py)

## Files

```
firmware/
  pcr_control/                 Arduino sketch folder (open this in the Arduino IDE)
    pcr_control.ino            control loop: read NTC -> PID -> heater/fan, CSV log over USB
    Thermistor.*               Beta-equation conversion + smoothing
    PidController.*            PID, integral only active within 0.5 C, anti-windup
    HeaterDriver.*             PWM to heater or fan; upstream block model for duty
    CycleSequencer.*           denature/anneal/extend sequencing, hold timers
  test/test_pcr.cpp            unit tests (runs on a PC)
  Makefile                     make test
hardware/
  heater_driver.cir            LTspice deck for the MOSFET heater switch
  heater_driver_notes.md       schematic changes, MOSFET loss, NTC divider numbers
analysis/thermal_spc.py        step statistics + thermocouple comparison
```

## Running it

```bash
cd firmware && make test          # g++ unit tests: thermistor math, PID, driver interlock, sequencing
```

Flash: open `firmware/pcr_control/` in the Arduino IDE, choose the SAMD21 (Arduino Zero)
board, upload. The sketch prints one CSV line every 100 ms:
`ms,setpoint_c,block_c,effort,heater_duty,fan_duty,phase,cycle`.

Log a thermocouple in a water-filled tube at the same time (`ms,ref_c`), then:

```bash
python analysis/thermal_spc.py run01_control.csv --ref run01_ref.csv
```

Pin numbers, PID starting gains and the heater model coefficients come from the PocketPCR
V1.02 firmware and are marked where they're used.


## License

GPL-3.0, same as PocketPCR (see [`LICENSE`](LICENSE)).
