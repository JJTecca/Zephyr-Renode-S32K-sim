from __future__ import annotations

# Trains the denoising autoencoder on ONE fault class and PERSISTS everything
import argparse
import json
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn

from dataset import FEATURES, Split, load_all, ts_split  # same directory

TICK_HZ: float = 10.0  # convert FP count to per-hour

# Those numbers are extracted fromt he ae.pt artifact u=[8112, 0, 0.2, 0.001], o=[1, 1, 0.4, 0.14])
# normal -> [ 0.000,     0.000,   −0.501,  −0.015]
# faulty -> [−4216.0,  −1360.0,    1.995,  −0.015]

# Normal row
# input (4):     [0.000,  0.000, −0.501, −0.015]
# Linear1+ReLU (8): [0.189, 0.466, 0, 0.310, 0.101, 0.878, 0, 0]
# Linear2+ReLU (3): [0.000, 0.000, 0.000]
# Linear3+ReLU (8): [0, 0.135, 0.719, 0, 0, 0.642, 0.190, 0]
# Linear4  rebuild (4): [−0.000, −0.009, −0.408, −0.010]

# Faulty row
# input (4):     [−4216.0, −1360.0, 1.995, −0.015]
# Linear1+ReLU (8): [0, 363.2, 0, 0, 0, 0, 1124.3, 335.5]
# Linear2+ReLU (3): [573.5, 488.8, 0.000]
# Linear3+ReLU (8): [0, 762.2, 0, 0, 437.0, 0, 278.4, 0]
# Linear4  rebuild (4): [1.530, 33.522, −48.586, −696.057]
def build_ae(d: int) -> nn.Sequential:
    return nn.Sequential(
        nn.Linear(d, 8), nn.ReLU(),
        nn.Linear(8, 3), nn.ReLU(),   # bottleneck: forces it to learn normal's shape
        nn.Linear(3, 8), nn.ReLU(),
        nn.Linear(8, d),
    )


def train(split: Split, epochs: int, noise: float, seed: int) -> nn.Sequential:
    torch.manual_seed(seed)
    Xtr = torch.tensor(split.Xtr_normal, dtype=torch.float32)  # NORMAL rows only
    ae = build_ae(Xtr.shape[1])
    opt = torch.optim.Adam(ae.parameters(), lr=1e-3)
    loss_fn = nn.MSELoss()
    ae.train()
    for _ in range(epochs):
        opt.zero_grad()
        out = ae(Xtr + noise * torch.randn_like(Xtr))  # denoising objective
        loss_fn(out, Xtr).backward()
        opt.step()
    ae.eval()
    return ae


def recon_error(ae: nn.Sequential, X: np.ndarray) -> np.ndarray:
    with torch.no_grad():
        t = torch.tensor(X, dtype=torch.float32)
        return ((ae(t) - t) ** 2).mean(1).numpy()  # score = mean squared rebuild error


def fpr_per_hour(scores_normal: np.ndarray, thr: float) -> float:
    """False positives per hour over NORMAL operation, at threshold thr."""
    if len(scores_normal) == 0:
        return float("nan")
    fp = int((scores_normal > thr).sum())
    hours = len(scores_normal) / TICK_HZ / 3600.0
    return fp / hours if hours > 0 else float("nan")


def main() -> None:
    ap = argparse.ArgumentParser(description="Train + persist the denoising AE for Sprint 2.")
    ap.add_argument("--glob", default="datasets/memory_leak_*.csv")
    ap.add_argument("--out", default="ml/artifacts", help="artifact directory")
    ap.add_argument("--epochs", type=int, default=300)
    ap.add_argument("--noise", type=float, default=0.1)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--margin", type=float, default=1.10,
                    help="threshold = max(train-normal score) * margin (>=1.0)")
    ap.add_argument("--calib-size", type=int, default=256,
                    help="rows saved as the int8 PTQ calibration set")
    a = ap.parse_args()

    feat, files = load_all(a.glob)
    split = ts_split(feat, FEATURES)
    print(f"[data] {len(files)} file(s) | features={FEATURES}")
    print(f"[data] train-normal={len(split.Xtr_normal)} | "
          f"test={len(split.Xte)} (faulty={int(split.yte.sum())})")

    ae = train(split, a.epochs, a.noise, a.seed)

    # scores
    s_tr = recon_error(ae, split.Xtr_normal)            # train-normal
    s_te = recon_error(ae, split.Xte)                    # test (both classes)
    s_te_normal = s_te[split.yte == 0]

    thr = float(s_tr.max() * a.margin)
    fph_new = fpr_per_hour(s_te_normal, thr)
    fph_old = fpr_per_hour(s_te_normal, float(np.quantile(s_tr, 0.99)))
    print(f"[thresh] policy=max*{a.margin:g} -> thr={thr:.6g}")
    print(f"[thresh] FP/hour: new-policy={fph_new:.2f}  (old q=0.99 was {fph_old:.2f})")

    # calibration set for int8 PTQ
    rng = np.random.default_rng(a.seed)
    pool = np.vstack([split.Xtr_normal, split.Xte])
    idx = rng.choice(len(pool), size=min(a.calib_size, len(pool)), replace=False)
    calib = pool[idx].astype(np.float32)

    # save
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "state_dict": ae.state_dict(),
            "arch": [split.Xtr_normal.shape[1], 8, 3, 8, split.Xtr_normal.shape[1]],
            "features": FEATURES,
            "scaler_mu": split.mu.tolist(),   # int8 model MUST use identical scaling
            "scaler_sd": split.sd.tolist(),
            "threshold": thr,
        },
        out / "ae.pt",
    )
    np.save(out / "calib.npy", calib)
    manifest = {
        "glob": a.glob,
        "files": files,
        "seed": a.seed,
        "epochs": a.epochs,
        "noise": a.noise,
        "features": FEATURES,
        "scaler_mu": split.mu.tolist(),
        "scaler_sd": split.sd.tolist(),
        "threshold": thr,
        "threshold_policy": f"max(train-normal score) * {a.margin}",
        "fp_per_hour_at_threshold": fph_new,
        "train_normal_rows": int(len(split.Xtr_normal)),
        "test_faulty_rows": int(split.yte.sum()),
        "calib_rows": int(len(calib)),
    }
    (out / "ae_manifest.json").write_text(json.dumps(manifest, indent=2))
    print(f"[save] {out/'ae.pt'}  {out/'calib.npy'}  {out/'ae_manifest.json'}")


if __name__ == "__main__":
    main()