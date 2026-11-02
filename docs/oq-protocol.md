# Thermal performance test (OQ-style)

**Unit:** PocketPCR main board + bottom heater + lid, running `firmware/pcr_control`.

## Acceptance criteria

| ID | Requirement | Limit |
|---|---|---|
| AC-1 | Hold accuracy at 95 / 60 / 72 °C (reference thermocouple, settled hold) | within ±1.0 °C |
| AC-2 | Hold stability within each hold (board sensor) | SD ≤ 0.3 °C |
| AC-3 | Overshoot after each step change | ≤ 2.0 °C |
| AC-4 | Settling time into ±0.5 °C | ≤ 15 s |
| AC-5 | Anneal hold means over 30 cycles | none outside I-MR control limits |
| AC-6 | NTC unplugged mid-run | heater, fan and lid off within one 100 ms period |

## Setup

- Fine Type-K/T thermocouple bead in a 0.2 mL tube with 20 µL water, held in the same well
  every run by the printed fixture (drawn in AutoCAD from the heater-block DXF, modeled in SolidWorks).
- Thermocouple checked in ice water and boiling water on the test day.
- 5 V / 3 A USB-C supply. Record ambient temperature and measured heater resistance.

## Procedure

1. Start the reference logger (`ms,ref_c`), then the board's serial log.
2. Run the standard program: 95 °C 120 s → 30 × (95 °C 30 s, 60 °C 30 s, 72 °C 30 s) → 72 °C 300 s.
3. Save `runNN_control.csv` and `runNN_ref.csv`. Repeat for 3 cold-start runs.
4. AC-6: unplug the NTC at 72 °C and confirm both PWM outputs drop to zero in the log.
5. Analyse:
   `python analysis/thermal_spc.py runNN_control.csv --ref runNN_ref.csv --plot runNN_imr.png`

Any result outside a limit is logged as a deviation (what was seen, cause, change made,
re-test run number); the original log is kept.

## Run sheet

| Run | Date | Ambient °C | R_heater Ω | AC-1 | AC-2 | AC-3 | AC-4 | AC-5 | AC-6 |
|---|---|---|---|---|---|---|---|---|---|
| 01 | | | | | | | | | |
| 02 | | | | | | | | | |
| 03 | | | | | | | | | |
