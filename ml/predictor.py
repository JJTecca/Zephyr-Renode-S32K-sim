from __future__ import annotations

import argparse
import glob as _glob
from dataclasses import dataclass
from dataset import load_all

import numpy as np
import pandas as pd

def ramp_frame(feat: pd.DataFrame) -> pd.DataFrame:
    copy_feat = feat.copy()
    # we are cutting the big table into few columns
    mask = (copy_feat["faulty"]) & (copy_feat["true_ttf"] > 0.1) # keeping negative faulty elements
    ramp = copy_feat[mask]
    return ramp[["episode", "timestamp", "heap_free", "heap_free_slope", "true_ttf"]]

def ttf_split(ramp: pd.DataFrame, features=["heap_free", "heap_free_slope"], train_frac=0.6) -> tuple:
    eps = sorted(ramp["episode"].unique()) # eps=1
    if len(eps) > 1:                                  # many seeds: hold out whole episodes
        n_tr = max(1, round(len(eps) * train_frac))
        # Whether each element in the DataFrame is contained in values eps[:n_tr].
        tr = ramp[ramp.episode.isin(eps[:n_tr])]
        te = ramp[ramp.episode.isin(eps[n_tr:])]
    else:
        g = ramp.sort_values("timestamp")
        cut = int(len(g) * train_frac)
        tr, te = g.iloc[:cut], g.iloc[cut:]

    #print(ramp[ramp.episode.isin(eps[:1])])
    Xtr = tr[features].to_numpy(float)
    ytr = tr["true_ttf"].to_numpy(float)
    Xte = te[features].to_numpy(float)
    yte = te["true_ttf"].to_numpy(float)
    return Xtr, ytr, Xte, yte

def predict_analytic(X: np.ndarray) -> np.ndarray:
    heap_free = X[:, 0]                       # column 0 = bytes still free
    slope = X[:, 1]                           # column 1 = bytes/s (negative while draining)
    drain = np.maximum(-slope, 1e-6)          # flip sign -> drain rate; clamp to avoid /0
    return heap_free / drain                  # seconds until heap hits 0

def fit(Xtr: np.ndarray, ytr: np.ndarray):
    from sklearn.linear_model import LinearRegression
    return LinearRegression().fit(Xtr, ytr)   # learns ttf ~= heap_free / true_drain

def predict(model, X: np.ndarray) -> np.ndarray:
    return model.predict(X)

def lead_time_error(ttf_true: np.ndarray, ttf_hat: np.ndarray) -> dict:
    err = ttf_true - ttf_hat
    mae = float(np.mean(np.abs(err)))
    rmse = float(np.sqrt(np.mean(err ** 2)))
    return {"mae": mae, "rmse": rmse}

def report(name: str, ttf_true: np.ndarray, ttf_hat: np.ndarray) -> None:
    m = lead_time_error(ttf_true, ttf_hat)
    print(f"[{name:9s}] MAE={m['mae']:.3f}s  RMSE={m['rmse']:.3f}s")

def plot_predictions(yte, analytic_hat, linreg_hat) -> None:
    import matplotlib.pyplot as plt
    lo, hi = 0, float(yte.max())

    fig, ax = plt.subplots(figsize=(6, 6))
    ax.plot([lo, hi], [lo, hi], "k--", lw=1, label="perfect (y = x)")   # ideal line
    ax.scatter(yte, analytic_hat, s=18, alpha=0.7, label="analytic")
    ax.scatter(yte, linreg_hat,  s=18, alpha=0.7, label="linreg")
    #the gap
    ax.vlines(yte, analytic_hat, yte, color="tab:blue", alpha=0.4, lw=1)
    ax.set_xlabel("true time-to-OOM (s)")
    ax.set_ylabel("predicted time-to-OOM (s)")
    ax.set_title("Predicted vs true")
    ax.legend()
    ax.set_aspect("equal")
    plt.tight_layout()
    plt.show()

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--glob", default="datasets/memory_leak_*.csv")
    a = ap.parse_args()
    feat, files = load_all(a.glob)          # from dataset.py — same as baseline
    print("cols:", list(feat.columns))
    print("true_ttf notna:", feat["true_ttf"].notna().sum())
    print("true_ttf max :", feat["true_ttf"].max())
    print("faulty True  :", feat["faulty"].sum())
    print("both cond    :", ((feat["faulty"]) & (feat["true_ttf"] > 0.1)).sum())
    ramp = ramp_frame(feat)                  # filter
    Xtr, ytr, Xte, yte = ttf_split(ramp)     # split
    # analytic vs learned, then report MAE/RMSE
    report("analytic", yte, predict_analytic(Xte))     # zero-parameter physics baseline
    model = fit(Xtr, ytr)                              # learned baseline
    report("linreg", yte, predict(model, Xte))

    analytic_res = predict_analytic(Xte)
    linreg_res = predict(model, Xte)
    plot_predictions(yte, analytic_res, linreg_res)
if __name__ == "__main__":
    main()