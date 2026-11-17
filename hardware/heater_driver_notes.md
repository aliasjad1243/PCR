# Heater driver and sensor notes

Numbers behind the schematic changes and the LTspice deck (`heater_driver.cir`).
Heater resistance is measured per board; 2.5 Ω is the example used below.

## Schematic changes (main board, `RocketPCR.sch`)

| Change | Why |
|---|---|
| Logic-level N-MOSFET for Q1–Q3 (R_DS(on) rated at V_GS = 2.5 V, e.g. AO3400A) | gates are driven straight from a 3.3 V SAMD21 pin |
| 100 kΩ gate pull-down on each MOSFET | keeps the heater off while the MCU is in reset or unprogrammed |
| Flyback diode across the fan connector | the fan is an inductive load on a low-side switch; none was drawn |
| R4 / R6 (4.7 kΩ NTC series resistors) added to the BOM export | they are on the schematic but missing from `RocketPCR.csv` |

## Heater current and MOSFET loss

```
I_heater = 5.0 V / 2.5 Ω            = 2.0 A      (P = 10 W)
P_fet    = I² · R_DS(on) = 2.0² × 0.048 Ω = 0.19 W   (AO3400A, 48 mΩ max at V_GS = 2.5 V)
ΔT_j     = 0.19 W × 125 °C/W         ≈ 24 °C      (SOT-23 on minimum pad)
```

At 40 °C inside the case that's ~64 °C junction, far below 150 °C.
A 2.0 A heater needs a 5 V / 3 A USB-C source; below ~1.7 Ω the heater would exceed 3 A at full duty.

## LTspice deck

`heater_driver.cir` drives the gate with a 3.3 V, ~732 Hz PWM (SAMD21 `analogWrite` default)
through the GPIO's ~25 Ω output resistance plus a 100 Ω gate resistor, with the 100 kΩ pull-down.
`.meas` lines report on-state current, V_DS, MOSFET dissipation and gate rise/fall time
(View → SPICE Error Log). The MOSFET is a simple level-1 model fitted to roughly 29–44 mΩ
at V_GS = 3.3–2.5 V; swap in the vendor model for sign-off numbers.

## NTC divider (10 kΩ @ 25 °C, B = 3435, 4.7 kΩ to 3.3 V, 12-bit ADC)

| Block | R_NTC | V_ADC | Counts | Resolution |
|---|---|---|---|---|
| 25 °C | 10.0 kΩ | 2.245 V | 2786 | 0.029 °C/count |
| 60 °C | 2.98 kΩ | 1.281 V | 1589 | 0.033 °C/count |
| 72 °C | 2.08 kΩ | 1.013 V | 1257 | 0.040 °C/count |
| 95 °C | 1.12 kΩ | 0.634 V | 787 | 0.063 °C/count |

ADC resolution is far finer than the ±1 °C target; NTC tolerance and the lag between the
NTC and the sample are what matter, which is why the block is checked against a thermocouple.
