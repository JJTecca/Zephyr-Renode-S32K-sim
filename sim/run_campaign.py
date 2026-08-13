#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SIG = {1: "heap_free", 2: "heap_used"}


def coerce(v):
    for cast in (int, float):
        try:
            return cast(v)
        except ValueError:
            pass
    return v


def read_config(path):
    cfg = dict(leak_bytes_per_tick=128, tick_hz=10)
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


def find_inject_ms(rows, explicit):
    """Inject moment: given, else the first non-zero heap_used sample."""
    if explicit is not None:
        return explicit
    for (t, sig, val) in rows:
        if sig == 2 and val > 0:
            return t
    return None


def label(rows, inject_ms, rate_per_s, out_csv):
    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp", "signal", "value", "label", "true_ttf"])
        for (t, sig, val) in rows:
            faulty = inject_ms is not None and t >= inject_ms
            lab = "faulty" if faulty else "normal"
            # analytical ground truth: heap_free / drain-rate
            ttf = round(val / rate_per_s, 3) if (faulty and sig == 1) else ""
            w.writerow([t, SIG.get(sig, sig), val, lab, ttf])
    return len(rows)

# Label a captured K1 UART telemetry log into a reproducible dataset CSV.
# python sim/run_campaign.py --log D:/.../k1_telem.log
#
def main():
    ap = argparse.ArgumentParser(description="Label a K1 UART log into a dataset CSV.")
    ap.add_argument("--log", required=True, help="captured K1 UART telemetry log")
    ap.add_argument("--config", default=str(REPO / "sim/configs/memory_leak.yaml"))
    ap.add_argument("--inject-ms", type=int, default=None,
                    help="uptime(ms) the leak was injected; omit to auto-detect")
    ap.add_argument("--out", default=None, help="output CSV path")
    ap.add_argument("--seed", type=int, default=42, help="tag only (filename)")
    args = ap.parse_args()

    cfg = read_config(args.config)
    rate_per_s = cfg["leak_bytes_per_tick"] * cfg["tick_hz"]

    rows = parse_rows(args.log)
    if not rows:
        raise SystemExit(f"no TELEM lines in {args.log}")

    inject_ms = find_inject_ms(rows, args.inject_ms)

    # Create logs dataset directory at paste the output there found in the UART
    (REPO / "datasets").mkdir(exist_ok=True)
    out_csv = str(REPO / "datasets" / f"memory_leak_leak{cfg['leak_bytes_per_tick']}_seed{args.seed}.csv")

    n = label(rows, inject_ms, rate_per_s, out_csv)
    where = "given" if args.inject_ms is not None else "auto-detected"
    print(f"[label] {n} rows | inject_ms={inject_ms} ({where}) -> {out_csv}")


if __name__ == "__main__":
    main()