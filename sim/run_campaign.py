#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SIG = {1: "heap_free", 2: "heap_used", 5: "loop_latency"}

# Per fault class
FAULTS = {
    "memory_leak":   dict(config="sim/configs/memory_leak.yaml",
                          onset_sig=2, onset_thresh=0,   # heap_used is EXACTLY 0 when healthy
                          ttf_sig=1, strength="leak_bytes_per_tick"),
    "deadline_miss": dict(config="sim/configs/timing_deadline_miss.yaml",
                          onset_sig=5, onset_thresh=1,   # loop_latency jitters 0<->1 when healthy
                          ttf_sig=None, strength="busy_spin_us"),
}


def coerce(v):
    for cast in (int, float):
        try:
            return cast(v)
        except ValueError:
            pass
    return v


def read_config(path):
    cfg = dict(leak_bytes_per_tick=128, busy_spin_us=20000, tick_hz=10)
    if path and Path(path).exists():
        for line in Path(path).read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if ":" in line:
                k, v = line.split(":", 1)
                cfg[k.strip()] = coerce(v.strip())
    return cfg


def parse_rows(uart_log):
    """[(t, sig, val), ...] from TELEM,uptime,node,sig,seq,val lines."""
    out = []
    for line in Path(uart_log).read_text(errors="ignore").splitlines():
        m = re.match(r"TELEM,(\d+),(\d+),(\d+),(\d+),(\d+)", line.strip())
        if m:
            t, _node, sig, _seq, val = map(int, m.groups())
            out.append((t, sig, val))
    return out


def find_inject_ms(rows, explicit, onset_sig):
    """Inject moment: given, else the first non-zero sample of the onset signal."""
    if explicit is not None:
        return explicit
    for (t, sig, val) in rows:
        # we can compare with 2 because the 0->1 transition could happen randomly because of clock
        # 2 or higher is already faulty
        if sig == onset_sig and val > 2:
            return t
    return None


def label(rows, inject_ms, rate_per_s, ttf_sig, out_csv):
    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)
        # true_ttf = heap_free ÷ leak_rate
        w.writerow(["timestamp", "signal", "value", "label", "true_ttf"])
        for (t, sig, val) in rows:
            faulty = inject_ms is not None and t >= inject_ms
            lab = "faulty" if faulty else "normal"
            # analytical ground truth only for prediction classes (ttf_sig set)
            ttf = (round(val / rate_per_s, 3)
                   if (faulty and ttf_sig is not None and sig == ttf_sig)
                   else "")
            w.writerow([t, SIG.get(sig, sig), val, lab, ttf])
    return len(rows)


def main():
    ap = argparse.ArgumentParser(description="Label a K1 UART log into a dataset CSV.")
    ap.add_argument("--log", required=True, help="captured K1 UART telemetry log")
    ap.add_argument("--fault", choices=list(FAULTS), default="memory_leak")
    ap.add_argument("--config", default=None, help="override the class default yaml")
    ap.add_argument("--inject-ms", type=int, default=None,
                    help="uptime(ms) the fault was injected; omit to auto-detect")
    ap.add_argument("--seed", type=int, default=42, help="tag only (filename)")
    ap.add_argument("--rate", type=int, default=None, help="different leak rates AS TESTING")
    args = ap.parse_args()

    spec = FAULTS[args.fault]
    cfg = read_config(args.config or str(REPO / spec["config"]))
    if args.rate is not None:
        cfg["leak_bytes_per_tick"] = args.rate
    rate_per_s = cfg["leak_bytes_per_tick"] * cfg["tick_hz"]   # only used by leak

    rows = parse_rows(args.log)
    if not rows:
        raise SystemExit(f"no TELEM lines in {args.log}")

    inject_ms = find_inject_ms(rows, args.inject_ms, spec["onset_sig"])

    # Create logs dataset directory at paste the output there found in the UART
    (REPO / "datasets").mkdir(exist_ok=True)
    # Find a proper file name for the csv i.e "memory_leak_128_seed42.csv"
    strength = cfg.get(spec["strength"], "x")
    out_csv = str(REPO / "datasets" / f"{args.fault}_{strength}_seed{args.seed}.csv")

    # we have the option to auto detect or to give args for ms interval
    n = label(rows, inject_ms, rate_per_s, spec["ttf_sig"], out_csv)
    where = "given" if args.inject_ms is not None else "auto-detected"
    print(f"[label] {args.fault} | {n} rows | inject_ms={inject_ms} ({where}) -> {out_csv}")


if __name__ == "__main__":
    main()