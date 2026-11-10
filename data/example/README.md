# Example data (PCR)

**⚠ Simulated data, not measurements.** Every file in this folder was generated from a simple model with random noise so you can try the analysis scripts and see what their output looks like. None of it was recorded from the device. Real results go in `data/` (outside this folder).

## Files

- `simulated_run_control.csv`: the sketch's 100 ms CSV log for a short program (95 °C, then 3 × 95/60/72 °C, then 72 °C)
- `simulated_run_ref.csv`: matching thermocouple log (`ms,ref_c`)

## Try it

```
python analysis/thermal_spc.py data/example/simulated_run_control.csv --ref data/example/simulated_run_ref.csv
```

Output:

```
denature  steps=3  max overshoot=0.46 C  max settle=10.4 s  worst hold SD=0.105 C  never settled=0
anneal    steps=3  max overshoot=0.31 C  max settle=21.7 s  worst hold SD=0.088 C  never settled=0
extend    steps=3  max overshoot=0.45 C  max settle=5.5 s  worst hold SD=0.103 C  never settled=0
60 C: board - thermocouple = +0.09 C  (within +/-1 C)
72 C: board - thermocouple = +0.22 C  (within +/-1 C)
95 C: board - thermocouple = +0.37 C  (within +/-1 C)
```
