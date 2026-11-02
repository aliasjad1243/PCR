"""Hold-temperature statistics and SPC chart for a PCR run.

    python thermal_spc.py run01_control.csv [--ref run01_ref.csv] [--plot run01_imr.png]

control log (from firmware): ms,setpoint_c,block_c,effort,heater_duty,fan_duty,phase,cycle
reference log (thermocouple): ms,ref_c
phase: 1 initial, 2 denature, 3 anneal, 4 extend, 5 final

Per step: overshoot, settling time into +/-0.5 C, hold mean and SD.
Across cycles: I-MR control limits on the anneal hold means (mean +/- 2.66 * MRbar).
With --ref: board-minus-thermocouple offset at each setpoint.
"""
import argparse
import csv
from statistics import mean, pstdev

NAMES = {1: "initial", 2: "denature", 3: "anneal", 4: "extend", 5: "final"}
BAND = 0.5


def load(path):
    with open(path) as fh:
        rows = csv.DictReader(l for l in fh if not l.startswith("#"))
        return [(float(r["ms"]) / 1000, float(r["setpoint_c"]), float(r["block_c"]), int(r["phase"]), int(r["cycle"]))
                for r in rows]


def steps(samples):
    out = []
    for s in samples:
        if out and s[1] == out[-1][-1][1] and s[3] == out[-1][-1][3]:
            out[-1].append(s)
        else:
            out.append([s])
    return out


def step_stats(step, start_temp):
    sp = step[0][1]
    temps = [s[2] for s in step]
    over = max(0.0, max(temps) - sp) if sp >= start_temp else max(0.0, sp - min(temps))
    settle = None
    for i in range(len(step) - 1, -1, -1):        # last sample outside the band
        if abs(temps[i] - sp) > BAND:
            break
        settle = i
    hold = temps[settle:] if settle is not None else []
    return {
        "phase": NAMES.get(step[0][3], "?"), "cycle": step[0][4], "sp": sp, "overshoot": over,
        "settle_s": None if settle is None else step[settle][0] - step[0][0],
        "mean": mean(hold) if len(hold) > 1 else None, "sd": pstdev(hold) if len(hold) > 1 else None,
    }


def imr(values):
    mr = mean(abs(b - a) for a, b in zip(values, values[1:]))
    c = mean(values)
    ucl, lcl = c + 2.66 * mr, c - 2.66 * mr
    return c, ucl, lcl, [i + 1 for i, v in enumerate(values) if v > ucl or v < lcl]


def ref_offsets(samples, ref):
    out = {}
    j = 0
    for t, sp, temp, *_ in samples:
        if abs(temp - sp) > BAND:
            continue
        while j < len(ref) - 2 and ref[j + 1][0] < t:
            j += 1
        (t0, r0), (t1, r1) = ref[j], ref[j + 1]
        if t0 <= t <= t1:
            out.setdefault(sp, []).append(temp - (r0 + (r1 - r0) * (t - t0) / (t1 - t0)))
    return {sp: mean(d) for sp, d in out.items()}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("control")
    ap.add_argument("--ref")
    ap.add_argument("--plot")
    a = ap.parse_args()

    samples = load(a.control)
    results, prev = [], samples[0][2]
    for st in steps(samples):
        results.append(step_stats(st, prev))
        prev = st[-1][2]

    for name in ("denature", "anneal", "extend"):
        rs = [r for r in results if r["phase"] == name]
        if not rs:
            continue
        settled = [r["settle_s"] for r in rs if r["settle_s"] is not None]
        sds = [r["sd"] for r in rs if r["sd"] is not None]
        print(f"{name:9s} steps={len(rs)}  max overshoot={max(r['overshoot'] for r in rs):.2f} C  "
              f"max settle={max(settled) if settled else float('nan'):.1f} s  "
              f"worst hold SD={max(sds) if sds else float('nan'):.3f} C  never settled={len(rs) - len(settled)}")

    anneal = [r["mean"] for r in results if r["phase"] == "anneal" and r["mean"] is not None]
    if len(anneal) > 1:
        c, ucl, lcl, ooc = imr(anneal)
        print(f"anneal I-MR: center {c:.3f}  UCL {ucl:.3f}  LCL {lcl:.3f}  out of control: {ooc or 'none'}")
        if a.plot:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
            fig, ax = plt.subplots(figsize=(8, 3.5))
            ax.plot(range(1, len(anneal) + 1), anneal, marker="o", lw=1)
            for y, ls in ((c, "-"), (ucl, "--"), (lcl, "--")):
                ax.axhline(y, ls=ls, color="gray", lw=1)
            ax.set_xlabel("cycle")
            ax.set_ylabel("anneal hold mean (C)")
            fig.tight_layout()
            fig.savefig(a.plot, dpi=150)

    if a.ref:
        with open(a.ref) as fh:
            ref = [(float(r["ms"]) / 1000, float(r["ref_c"])) for r in csv.DictReader(fh)]
        for sp, off in sorted(ref_offsets(samples, ref).items()):
            print(f"{sp:.0f} C: board - thermocouple = {off:+.2f} C")


if __name__ == "__main__":
    main()
