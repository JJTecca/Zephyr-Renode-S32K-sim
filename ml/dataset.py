from __future__ import annotations
import glob as _glob
from dataclasses import dataclass

import numpy as np
import pandas as pd

# Stable feature order used by every model downstream.
FEATURES = ["heap_free", "heap_free_slope", "loop_latency", "loop_latency_slope"]


def load_episode(csv, episode_id=0):
    """Read one labelled CSV (long format) -> tidy DataFrame."""
    df = pd.read_csv(csv)
    df["episode"] = episode_id
    return df


def pivot_wide(df):
    wide = df.pivot_table(index=["episode", "timestamp"], columns="signal",
                          values="value", aggfunc="first").reset_index()
    wide.columns.name = None
    lab = (df.assign(f=(df["label"] == "faulty"))
             .groupby(["episode", "timestamp"])["f"].max().reset_index())
    wide = wide.merge(lab, on=["episode", "timestamp"]).rename(columns={"f": "faulty"})
    for c in ("heap_free", "heap_used", "loop_latency"):
        if c not in wide.columns:      # a class may not emit every signal
            wide[c] = np.nan
    return wide.sort_values(["episode", "timestamp"]).reset_index(drop=True)


def _rolling_slope(series, window, dt_s):
    """Slope (units/second) over a trailing window (mean first-difference)."""
    return series.diff().rolling(window, min_periods=2).mean() / dt_s


def add_features(wide, window=10, dt_s=0.1):
    """Rolling-slope features, computed PER EPISODE (no cross-episode leakage)."""
    parts = []
    for _, g in wide.groupby("episode"):
        g = g.copy()
        g["heap_free_slope"] = _rolling_slope(g["heap_free"], window, dt_s)
        g["loop_latency_slope"] = _rolling_slope(g["loop_latency"], window, dt_s)
        parts.append(g)
    out = pd.concat(parts).reset_index(drop=True)
    out["heap_free"] = out["heap_free"].ffill().bfill()
    out["loop_latency"] = out["loop_latency"].fillna(0.0)
    out[["heap_free_slope", "loop_latency_slope"]] = \
        out[["heap_free_slope", "loop_latency_slope"]].fillna(0.0)
    return out


def load_all(pattern, window=10, dt_s=0.1):
    """Load + feature-engineer every CSV matching a glob into one frame."""
    files = sorted(_glob.glob(pattern))
    if not files:
        raise SystemExit(f"no CSVs match {pattern}")
    frames = [load_episode(f, i) for i, f in enumerate(files)]
    wide = pivot_wide(pd.concat(frames, ignore_index=True))
    return add_features(wide, window, dt_s), files


@dataclass
class Split:
    Xtr_normal: np.ndarray   # train features, NORMAL only (unsupervised fit)
    Xte: np.ndarray          # test features (normal + faulty)
    yte: np.ndarray          # test labels (1 = faulty)
    mu: np.ndarray           # scaler mean (fit on train-normal)
    sd: np.ndarray           # scaler std  (fit on train-normal)


def ts_split(feat, features=FEATURES, train_frac=0.6):
    """Time-series split (NEVER random). Multiple episodes -> hold out the last
    for test. Single episode -> train on the first train_frac of NORMAL; test on
    the remaining normal + all faulty (so the test set holds BOTH classes)."""
    eps = sorted(feat["episode"].unique())
    if len(eps) > 1:
        n_tr = max(1, int(round(len(eps) * train_frac)))
        tr = feat[feat["episode"].isin(eps[:n_tr])]
        te = feat[feat["episode"].isin(eps[n_tr:])]
        tr_norm = tr[~tr["faulty"]]
    else:
        g = feat.sort_values("timestamp")
        normal, faulty = g[~g["faulty"]], g[g["faulty"]]
        cut = int(len(normal) * train_frac)
        tr_norm = normal.iloc[:cut]
        te = pd.concat([normal.iloc[cut:], faulty]).sort_values("timestamp")

    Xtr = tr_norm[features].to_numpy(float)
    mu, sd = Xtr.mean(0), Xtr.std(0)                 # fit on train-normal ONLY
    sd[sd < 1e-9] = 1.0                              # leave constant cols unscaled
    return Split((Xtr - mu) / sd,
                 (te[features].to_numpy(float) - mu) / sd,
                 te["faulty"].to_numpy(int), mu, sd)


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--glob", default="datasets/*.csv")
    a = ap.parse_args()
    feat, files = load_all(a.glob)
    print(f"[dataset] {len(files)} file(s), {len(feat)} timestamps, "
          f"{int(feat['faulty'].sum())} faulty")
    print(feat[["episode", "timestamp"] + FEATURES + ["faulty"]].head(8).to_string(index=False))
    s = ts_split(feat)
    print(f"[split] train-normal={len(s.Xtr_normal)}  "
          f"test={len(s.Xte)}  test-faulty={int(s.yte.sum())}")