"""
ANALYZE FLIGHT (version 3): compares apogee at 100 Hz, 50 Hz and 10 Hz from ONE flight log.

How to run (Python 3, no extra installs needed):
    python analyze_flight.py LOG000.CSV

Set UP_AXIS and UP_SIGN below first (the accelerometer axis that read about
+/-9.8 with the rocket standing nose-up on the pad).
"""
import csv, sys, statistics

# ---------------- SETTINGS: CHANGE THESE ----------------
UP_AXIS = "ax_mps2"      # the accelerometer column that points to the nose
UP_SIGN = +1             # +1 if that axis read about +9.8 nose-up, -1 if about -9.8
LAUNCH_G = 3.0           # launch = acceleration above this many g ...
LAUNCH_SAMPLES = 5       # ... for this many readings in a row (ignores bumps from handling)
EJECT_G = 20.0           # ejection = first jolt above this many g after the motor burns out
RATES = [100, 50, 10]    # sampling rates to compare (Hz). Logger records at 100 Hz.
FIT_WINDOW_S = 1.0       # barometer apogee: fit a curve to +/- this many seconds around the peak
SEARCH_S = 30.0          # look for apogee within this many seconds after launch
# ---------------------------------------------------------

G = 9.80665

def load(path):
    rows = []
    with open(path, newline="") as f:
        for r in csv.DictReader(f):
            try:
                ax, ay, az = float(r["ax_mps2"]), float(r["ay_mps2"]), float(r["az_mps2"])
                rows.append({"t": float(r["t_s"]), "p": float(r["pressure_Pa"]),
                             "a": UP_SIGN * float(r[UP_AXIS]),
                             "mag": (ax * ax + ay * ay + az * az) ** 0.5})
            except (ValueError, KeyError):
                pass                         # skip a damaged last line
    # Barometer values are one sample late (library behavior): give each
    # pressure the time of the PREVIOUS row.
    for i in range(len(rows) - 1, 0, -1):
        rows[i]["tp"] = rows[i - 1]["t"]
    rows[0]["tp"] = rows[0]["t"]
    return rows[1:]

def find_events(rows):
    """Launch, burnout and ejection, found once from the full 100 Hz record."""
    n = 0; launch = None
    for i, r in enumerate(rows):
        n = n + 1 if r["a"] > LAUNCH_G * G else 0
        if n == LAUNCH_SAMPLES:
            launch = i - LAUNCH_SAMPLES + 1
            break
    if launch is None:
        sys.exit("No launch found. Check UP_AXIS / UP_SIGN or lower LAUNCH_G.")
    t_launch = rows[launch]["t"]
    pad = [r for r in rows if t_launch - 3.0 <= r["t"] < t_launch - 0.5]
    if len(pad) < 20:
        sys.exit("Need at least 3 seconds of pad data before launch.")
    a0 = statistics.mean(r["a"] for r in pad)
    burnout = next(i for i in range(launch, len(rows)) if rows[i]["a"] < a0)
    t_burn = rows[burnout]["t"]
    eject = next((i for i in range(burnout, len(rows))
                  if rows[i]["t"] > t_burn + 0.5 and rows[i]["mag"] > EJECT_G * G), None)
    t_eject = rows[eject]["t"] if eject is not None else t_launch + SEARCH_S
    return t_launch, t_burn, t_eject, pad

def baro_alt(p, p0):
    return 44330.0 * (1.0 - (p / p0) ** 0.1903)

def fit_peak(points, t_center):
    """Least-squares parabola through (time, height) points near the peak.
    Returns (peak height, peak time); falls back to the highest point."""
    top = max(points, key=lambda q: q[1])
    if len(points) < 5:
        return top[1], top[0]
    S = [0.0] * 5; T = [0.0] * 3
    for t, h in points:
        x = t - t_center
        for k in range(5): S[k] += x ** k
        for k in range(3): T[k] += h * x ** k
    M = [[S[4], S[3], S[2], T[2]], [S[3], S[2], S[1], T[1]], [S[2], S[1], S[0], T[0]]]
    for i in range(3):
        piv = max(range(i, 3), key=lambda r: abs(M[r][i]))
        M[i], M[piv] = M[piv], M[i]
        if abs(M[i][i]) < 1e-12:
            return top[1], top[0]
        for r in range(3):
            if r != i:
                f = M[r][i] / M[i][i]
                M[r] = [M[r][c] - f * M[i][c] for c in range(4)]
    a, b, c = (M[i][3] / M[i][i] for i in range(3))
    if a >= 0 or abs(-b / (2 * a)) > FIT_WINDOW_S:
        return top[1], top[0]
    return c - b * b / (4 * a), t_center - b / (2 * a)

def analyze(samples, t_launch, t_burn, t_eject, p0, a0):
    # --- barometer apogee (curve fit near the highest reading) ---
    flight = [r for r in samples if t_launch <= r["tp"] <= t_launch + SEARCH_S]
    best = max(flight, key=lambda r: baro_alt(r["p"], p0))
    win = [(r["tp"], baro_alt(r["p"], p0)) for r in flight
           if abs(r["tp"] - best["tp"]) <= FIT_WINDOW_S]
    baro_apogee, _ = fit_peak(win, best["tp"])
    # --- accelerometer: integrate from launch until apogee or ejection ---
    v = h = 0.0; peak = 0.0; v_burn = h_burn = None
    seg = [r for r in samples if t_launch - 0.2 <= r["t"] < t_eject]
    acc_apogee = None
    for prev, cur in zip(seg, seg[1:]):
        dt = cur["t"] - prev["t"]
        v_new = v + 0.5 * ((prev["a"] - a0) + (cur["a"] - a0)) * dt   # trapezoid rule
        h += 0.5 * (v + v_new) * dt
        v = v_new
        peak = max(peak, cur["a"] / G)
        if v_burn is None and cur["t"] >= t_burn:
            v_burn, h_burn = v, h
        if v <= 0 and cur["t"] > t_burn:        # stopped climbing before ejection
            acc_apogee = h
            break
    if acc_apogee is None:                      # ejected while still climbing:
        acc_apogee = h + v * v / (2 * G)        # add the remaining coast (v^2 / 2g)
    return baro_apogee, acc_apogee, peak, v_burn, h_burn

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: python analyze_flight.py LOGxxx.CSV")
    rows = load(sys.argv[1])
    t_launch, t_burn, t_eject, pad = find_events(rows)
    p0 = statistics.mean(r["p"] for r in pad)
    a0 = statistics.mean(r["a"] for r in pad)
    print(f"File: {sys.argv[1]}   rows: {len(rows)}")
    print(f"Launch t = {t_launch:.2f} s   burn time = {t_burn - t_launch:.2f} s   "
          f"ejection {t_eject - t_launch:.2f} s after launch")
    print(f"Up axis: {UP_AXIS} (sign {UP_SIGN:+d}); 1 g on the pad read as {a0:.2f} m/s^2\n")
    hdr = "Rate   Baro apogee (m)      Accel apogee (m)     Peak accel (g)      Burnout speed (m/s)"
    print(hdr); print("-" * len(hdr))
    out = []
    for hz in RATES:
        step = round(100 / hz)
        res = [analyze(rows[off::step], t_launch, t_burn, t_eject, p0, a0) for off in range(step)]
        def rng(vals): return f"{statistics.mean(vals):7.1f} ({min(vals):.1f}-{max(vals):.1f})"
        cols = [[r[i] for r in res] for i in range(4)]
        print(f"{hz:>3} Hz " + "  ".join(f"{rng(c):>20}" for c in cols))
        for off, r in enumerate(res):
            out.append([hz, off] + [round(x, 3) for x in r])
    with open("apogee_results.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["rate_hz", "start_row", "baro_apogee_m", "accel_apogee_m",
                    "peak_accel_g", "burnout_speed_mps", "accel_alt_at_burnout_m"])
        w.writerows(out)
    print("\nNumbers in brackets = lowest-highest over every possible starting row.")
    print("Every individual result was saved to apogee_results.csv")

if __name__ == "__main__":
    main()
