#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, re, subprocess, tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
RENODE_CANDIDATES = [r"D:\Renode\bin\Renode.exe", r"D:\Renode\Renode.exe"]
SIG = {1: "heap_free", 2: "heap_used"}


def find_renode(explicit):
    if explicit:
        return explicit
    for c in RENODE_CANDIDATES:
        if Path(c).exists():
            return c
    return "renode"  # assume on PATH


def coerce(v):
    for cast in (int, float):
        try:
            return cast(v)
        except ValueError:
            pass
    return v


def read_config(path, args):
    cfg = dict(node="k1", signal="heap_free", baseline_s=3,
               leak_bytes_per_tick=128, tick_hz=10, run_s=12)
    if path and Path(path).exists():                       # tiny flat-YAML reader (no pyyaml)
        for line in Path(path).read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if ":" in line:
                k, v = line.split(":", 1)
                cfg[k.strip()] = coerce(v.strip())
    if args.leak is not None:      cfg["leak_bytes_per_tick"] = args.leak
    if args.baseline is not None:  cfg["baseline_s"] = args.baseline
    if args.run is not None:       cfg["run_s"] = args.run
    return cfg


def build_resc(cfg, uart_log):
    repl  = (REPO / "sim/renode/k1_edge.repl").as_posix()
    elf   = (REPO / "build/k1_powertrain/zephyr/zephyr.elf").as_posix()
    hooks = (REPO / "sim/renode/fault_hooks.py").as_posix()
    node, leak = cfg["node"], cfg["leak_bytes_per_tick"]
    rest = cfg["run_s"] - cfg["baseline_s"]
    return f'''mach create "{node}"
machine LoadPlatformDescription @{repl}
sysbus LoadELF @{elf}
cpu0 VectorTableOffset `sysbus GetSymbolAddress "_vector_table"`
sysbus.lpuart2 CreateFileBackend @{Path(uart_log).as_posix()}
i @{hooks}
mach set "{node}"
emulation RunFor "{cfg["baseline_s"]}"
inject_memory_leak {node} {leak}
emulation RunFor "{rest}"
quit
'''


def parse_and_label(uart_log, cfg, out_csv):
    inject_ms  = cfg["baseline_s"] * 1000
    rate_per_s = cfg["leak_bytes_per_tick"] * cfg["tick_hz"]
    rows = []
    for line in Path(uart_log).read_text(errors="ignore").splitlines():
        m = re.match(r"TELEM,(\d+),(\d+),(\d+),(\d+),(\d+)", line.strip())
        if not m:
            continue
        t, _node, sig, _seq, val = map(int, m.groups())
        label = "normal" if t < inject_ms else "faulty"
        ttf = round(val / rate_per_s, 3) if (label == "faulty" and sig == 1) else ""
        rows.append((t, SIG.get(sig, sig), val, label, ttf))
    with open(out_csv, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["timestamp", "signal", "value", "label", "true_ttf"])
        w.writerows(rows)
    return len(rows)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--config", default=str(REPO / "sim/configs/memory_leak.yaml"))
    ap.add_argument("--leak", type=int)
    ap.add_argument("--baseline", type=float)
    ap.add_argument("--run", type=float)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--renode")
    args = ap.parse_args()

    cfg = read_config(args.config, args)
    renode = find_renode(args.renode)
    (REPO / "datasets").mkdir(exist_ok=True)
    out_csv = REPO / "datasets" / f"memory_leak_leak{cfg['leak_bytes_per_tick']}_seed{args.seed}.csv"

    with tempfile.TemporaryDirectory() as td:
        uart_log = Path(td) / "telemetry.log"
        resc = Path(td) / "campaign.resc"
        resc.write_text(build_resc(cfg, uart_log))
        print(f"[campaign] renode: {renode}")
        subprocess.run([renode, "--disable-gui", "--console", str(resc)], check=True)
        n = parse_and_label(uart_log, cfg, out_csv)

    print(f"[campaign] wrote {n} rows -> {out_csv}")


if __name__ == "__main__":
    main()