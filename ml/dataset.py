from __future__ import annotations

import glob as _glob
from dataclasses import dataclass

import numpy as np
import pandas as pd

# Stable feature order used by every model downstream.
FEATURES: list[str] = ["heap_free", "heap_free_slope", "loop_latency", "loop_latency_slope"]


def load_episode(csv: str, episode_id: int = 0) -> pd.DataFrame:
    """Read one labelled CSV (long format) -> tidy DataFrame."""
    df = pd.read_csv(csv)
    df["episode"] = episode_id
    # what df looks like now (LONG: one row per signal-sample, tagged episode):
    #   timestamp  signal        value  label   true_ttf  episode
    #   200        heap_free     8112   normal            0
    #   201        heap_used     0      normal            0
    #   200        loop_latency  0      normal            0
    #   400        heap_free     8000   faulty  6.25       0
    #   401        heap_used     112    faulty            0
    return df


def pivot_wide(df: pd.DataFrame) -> pd.DataFrame:
    df = df.copy()
    # snap every timestamp to the nearest 100 ms tick so they land on the SAME row
    # instead of pivoting into separate half-empty (NaN) rows.
    df["timestamp"] = (df["timestamp"] / 100).round().astype(int) * 100
    #   BEFORE (long):                    AFTER pivot (wide, one row per tick):
    #   timestamp  signal      value      episode timestamp heap_free heap_used loop_latency
    #   200        heap_free   8112       0       200       8112      0         0
    #   200        heap_used   0          0       400       8000      112       60
    #   200        loop_latency 0
    wide = df.pivot_table(index=["episode", "timestamp"], columns="signal",
                          values="value", aggfunc="first").reset_index()
    wide.columns.name = None
    #   episode  timestamp  f
    #   0        200        False
    #   0        400        True
    lab = (df.assign(f=(df["label"] == "faulty"))
             .groupby(["episode", "timestamp"])["f"].max().reset_index())
    wide = wide.merge(lab, on=["episode", "timestamp"]).rename(columns={"f": "faulty"})
    ttf = df.groupby(["episode", "timestamp"])["true_ttf"].max().reset_index()
    wide = wide.merge(ttf, on=["episode", "timestamp"])
    for c in ("heap_free", "heap_used", "loop_latency"):
        if c not in wide.columns:      # a class may not emit every signal
            wide[c] = np.nan
    # wide looks like now (one clean row per 100 ms tick):
    #   episode  timestamp  heap_free  heap_used  loop_latency  faulty
    #   0        200        8112       0          0             False
    #   0        400        8000       112        60            True
    return wide.sort_values(["episode", "timestamp"]).reset_index(drop=True)


def _rolling_slope(series: pd.Series, window: int, dt_s: float) -> pd.Series:
    """Slope (units/second) over a trailing window (mean first-difference)."""
    # check the series columns from the above table and subtract "v[i]-v[i+1]"
    return series.diff().rolling(window, min_periods=2).mean() / dt_s #dt_s = changes per s


def add_features(wide: pd.DataFrame, window: int = 10, dt_s: float = 0.1) -> pd.DataFrame:
    """Rolling-slope features, computed PER EPISODE (no cross-episode leakage)."""
    parts: list[pd.DataFrame] = []
    for _, g in wide.groupby("episode"):
        g = g.copy()
        g["heap_free_slope"] = _rolling_slope(g["heap_free"], window, dt_s)
        g["loop_latency_slope"] = _rolling_slope(g["loop_latency"], window, dt_s)
        parts.append(g)
    out = pd.concat(parts).reset_index(drop=True)
    # Use ffill() function to fill the missing values along the index axis. 
    # bfill() = Fill NA/NaN values by using the next valid observation to fill the gap
    out["heap_free"] = out["heap_free"].ffill().bfill()
    out["loop_latency"] = out["loop_latency"].fillna(0.0)
    out[["heap_free_slope", "loop_latency_slope"]] = \
        out[["heap_free_slope", "loop_latency_slope"]].fillna(0.0)
    #   timestamp  heap_free  heap_free_slope  loop_latency  loop_latency_slope  faulty
    #   200        8112       0.0              0             0.0                 False
    #   400        8000       -560.0           60            300.0               True
    #   600        7872       -1200.0          60            0.0                 True
    return out


def load_all(pattern: str, window: int = 10, dt_s: float = 0.1) -> tuple[pd.DataFrame, list[str]]:
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


def ts_split(feat: pd.DataFrame, features: list[str] = FEATURES,
             train_frac: float = 0.6) -> Split:
    # Single episode = train on the first train_frac of NORMAL; test on
    # the remaining normal + all faulty (so the test set holds BOTH classes)."""
    eps = sorted(feat["episode"].unique())
    if len(eps) > 1:
        n_tr = max(1, int(round(len(eps) * train_frac)))
        # isin() = Whether a value matches something in a list
        tr = feat[feat["episode"].isin(eps[:n_tr])]
        te = feat[feat["episode"].isin(eps[n_tr:])]
        tr_norm = tr[~tr["faulty"]]
    else:
        g = feat.sort_values("timestamp")
        normal, faulty = g[~g["faulty"]], g[g["faulty"]]
        cut = int(len(normal) * train_frac)
        # everything after it
        tr_norm = normal.iloc[:cut]
        te = pd.concat([normal.iloc[cut:], faulty]).sort_values("timestamp")

    # at this point the frame becomes plain numpy matrices (rows x 4 features):
    #   Xtr (train-normal only)        yte (test labels)
    #   [[8112,   0, 0,   0],          [0, 0, ... 1, 1]   (0=normal, 1=faulty)
    #    [8100, -60, 0,   0], ...]     test rows carry BOTH classes
    # each column is then standardized: z = (x - mu) / sd, mu/sd from train-normal.
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