from __future__ import annotations

import argparse

import numpy as np
from sklearn.ensemble import IsolationForest
from sklearn.metrics import roc_auc_score

from dataset import FEATURES, Split, load_all, ts_split  # same directory

TICK_HZ: float = 10.0  # telemetry cadence -> convert FP count to per-hour


def threshold_from_train(train_scores: np.ndarray, q: float = 0.99) -> float:
    """Operating point: the q-quantile of TRAIN-normal scores."""
    return float(np.quantile(train_scores, q))


def fpr_per_hour(y_true: np.ndarray, scores: np.ndarray, thr: float) -> float:
    """False positives per hour of NORMAL operation."""
    normal = scores[y_true == 0]
    if len(normal) == 0:
        return float("nan")
    fp = int((normal > thr).sum())
    hours = len(normal) / TICK_HZ / 3600.0
    return fp / hours if hours > 0 else float("nan")


def run_isoforest(split: Split) -> tuple[np.ndarray, np.ndarray]:
    """Fit Isolation Forest on train-normal; return (train, test) anomaly scores."""
    clf = IsolationForest(n_estimators=200, contamination="auto", random_state=42)
    clf.fit(split.Xtr_normal)
    # scores it returns (higher = more anomalous). On perfectly-flat normal data
    # the forest has nothing to split on, so scores collapse near a constant ~0.5
    # and it can't tell normal from faulty (ROC-AUC ~0.5 -> needs realistic noise):
    #   s_tr = [0.50, 0.50, 0.49, ...]      (train-normal)
    #   s_te = [0.50, 0.51, 0.50, 0.50]     (test: normal AND faulty look alike)
    return -clf.score_samples(split.Xtr_normal), -clf.score_samples(split.Xte)


def run_autoencoder(split: Split, epochs: int = 300, noise: float = 0.1,
                    seed: int = 42) -> tuple[np.ndarray, np.ndarray]:
    """Train a small denoising autoencoder on train-normal; score = recon error."""
    import torch
    import torch.nn as nn
    torch.manual_seed(seed)

    Xtr = torch.tensor(split.Xtr_normal, dtype=torch.float32)
    Xte = torch.tensor(split.Xte, dtype=torch.float32)
    d = Xtr.shape[1]
    ae = nn.Sequential(
        nn.Linear(d, 8), nn.ReLU(),
        nn.Linear(8, 3), nn.ReLU(),      # bottleneck
        nn.Linear(3, 8), nn.ReLU(),
        nn.Linear(8, d),
    )
    opt = torch.optim.Adam(ae.parameters(), lr=1e-3)
    loss_fn = nn.MSELoss()
    ae.train()
    for _ in range(epochs):
        opt.zero_grad()
        out = ae(Xtr + noise * torch.randn_like(Xtr))    # denoising objective
        loss_fn(out, Xtr).backward()
        opt.step()
    ae.eval()
    with torch.no_grad():
        s_tr = ((ae(Xtr) - Xtr) ** 2).mean(1).numpy()    # reconstruction error
        s_te = ((ae(Xte) - Xte) ** 2).mean(1).numpy()
    # scores = how badly the AE (trained ONLY on normal) rebuilds each row.
    # normal rows rebuild near-perfectly (~0), faulty rows it has never seen blow
    # up -> clean separation, ROC-AUC ~1.0:
    #   s_tr = [0.001, 0.000, 0.002, ...]        (normal: tiny error)
    #   s_te = [0.001, 0.002, 9.1e6, 8.7e6]      (faulty tail: huge error)
    return s_tr, s_te


def report(name: str, split: Split, s_tr: np.ndarray, s_te: np.ndarray) -> None:
    """Print ROC-AUC + FP/hour for one detector's scores."""
    if split.yte.sum() in (0, len(split.yte)):
        print(f"[{name}] test has one class only -- ROC undefined "
              f"(add more/varied episodes)")
        return
    # thr = the 0.99 quantile of TRAIN-normal scores = the alarm line. Anything in
    # the test set scoring above it is flagged faulty:
    #   thr = 0.002
    #   s_te = [0.001, 0.002, 9.1e6, 8.7e6]  ->  flagged = [ .    .    ALARM  ALARM ]
    # ROC-AUC = separability over all thresholds (1.0 perfect); FP/hour = false
    # alarms per hour of normal running.
    auc = roc_auc_score(split.yte, s_te)
    thr = threshold_from_train(s_tr, q=0.99)
    fph = fpr_per_hour(split.yte, s_te, thr)
    print(f"[{name:11s}] ROC-AUC={auc:.3f}  FP/hour={fph:.2f}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--glob", default="datasets/memory_leak_*.csv")
    ap.add_argument("--epochs", type=int, default=300)
    a = ap.parse_args()

    feat, files = load_all(a.glob)
    split = ts_split(feat, FEATURES)
    print(f"[data] {len(files)} file(s) | features={FEATURES}")
    print(f"[data] train-normal={len(split.Xtr_normal)} | "
          f"test={len(split.Xte)} (faulty={int(split.yte.sum())})")

    s_tr, s_te = run_isoforest(split)
    report("iso-forest", split, s_tr, s_te)

    s_tr, s_te = run_autoencoder(split, epochs=a.epochs)
    report("denoise-AE", split, s_tr, s_te)


if __name__ == "__main__":
    main()