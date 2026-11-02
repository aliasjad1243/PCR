# Portable PCR Machine

My contributions to **[PocketPCR](https://github.com/GaudiLabs/PocketPCR)**, an open-source,
USB-powered thermal cycler by Urs Gaudenz (GaudiLabs). A PCR machine copies DNA by cycling a
small heated block between about 95 °C (denature), 60 °C (anneal) and 72 °C (extend), 30 or
more times, so everything comes down to how well the block hits and holds temperature.

I built the machine from the PocketPCR design, then reworked the heater driver, restructured
the control firmware, and tested how well it holds temperature.
**This repo only holds the files I wrote or changed.** The KiCad boards, Gerbers and original
firmware are in the PocketPCR repo.

## The original project

| | |
|---|---|
| Repo | [GaudiLabs/PocketPCR](https://github.com/GaudiLabs/PocketPCR) (GPL-3.0) · [gaudi.ch/PocketPCR](http://gaudi.ch/PocketPCR/) |
| Boards | main board (SAMD21, USB-C, OLED, rotary encoder), PCB-trace bottom heater with NTC, heated lid |
| Firmware | single Arduino sketch, V1.02 |

## What I did

- **Heater driver:** changed the main-board schematic in KiCad (logic-level MOSFETs, gate
  pull-downs, fan flyback diode) and simulated the low-side heater switch in LTspice
  → [`hardware/`](hardware/)
- **Firmware refactor:** split the single sketch into small C++ classes: `Thermistor` (ADC →
  °C), `PidController`, `HeaterDriver` (heater/fan PWM, never both on) and `CycleSequencer`
  (the PCR program), with host-side unit tests → [`firmware/`](firmware/)
- **Temperature check:** a thermocouple in a water-filled tube, held by a fixture drafted in
  AutoCAD from the heater-block DXF and modeled in SolidWorks, as the reference
- **Validation:** tuned the PID heater and PWM fan control and wrote an OQ-style test protocol
  with acceptance limits (95 / 60 / 72 °C within ±1 °C) → [`docs/oq-protocol.md`](docs/oq-protocol.md)
- **SPC:** Python script for overshoot, settling time and hold SD per step, and an I-MR control
  chart of hold temperatures across 30 cycles → [`analysis/thermal_spc.py`](analysis/thermal_spc.py)

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
docs/oq-protocol.md            acceptance criteria, procedure, run sheet
analysis/thermal_spc.py        step statistics, I-MR chart, thermocouple offset
```

## Running it

```bash
cd firmware && make test          # g++ unit tests: thermistor math, PID, driver interlock, sequencing
```

Flash: open `firmware/pcr_control/` in the Arduino IDE, choose the SAMD21 (Arduino Zero)
board, upload. The sketch prints one CSV line every 100 ms:
`ms,setpoint_c,block_c,effort,heater_duty,fan_duty,phase,cycle`.

```bash
python analysis/thermal_spc.py run01_control.csv --ref run01_ref.csv --plot run01_imr.png
```

Pin numbers, PID starting gains and the heater model coefficients come from the PocketPCR
V1.02 firmware and are marked where they're used.

## License

GPL-3.0, same as PocketPCR (see [`LICENSE`](LICENSE)).
