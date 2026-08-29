from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
from sklearn.metrics import roc_auc_score

from dataset import FEATURES, load_all, ts_split  # same directory


def build_ae(arch: list[int]) -> nn.Sequential:
    d = arch[0]
    return nn.Sequential(
        nn.Linear(d, 8), nn.ReLU(),
        nn.Linear(8, 3), nn.ReLU(),
        nn.Linear(3, 8), nn.ReLU(),
        nn.Linear(8, d),
    )


def recon_error(ae: nn.Sequential, X: np.ndarray) -> np.ndarray:
    with torch.no_grad():
        t = torch.tensor(X, dtype=torch.float32)
        return ((ae(t) - t) ** 2).mean(1).numpy()


def quantize_weights(ae: nn.Sequential) -> dict:
    """Per-tensor symmetric int8: q = round(w/scale), scale = max|w|/127.
    Writes dequantized weights back in place (simulated int8) and returns the
    int8 tensors + scales for export."""
    export = {}
    for i, layer in enumerate(ae):
        if not isinstance(layer, nn.Linear):
            continue
        w = layer.weight.detach().numpy()
        scale = float(np.abs(w).max()) / 127.0 or 1e-12
        q = np.clip(np.round(w / scale), -127, 127).astype(np.int8)
        layer.weight.data = torch.tensor(q.astype(np.float32) * scale)
        export[f"l{i}_w"] = q
        export[f"l{i}_scale"] = np.float32(scale)
    return export


def main() -> None:
    ap = argparse.ArgumentParser(description="int8 weight PTQ + parity (Sprint 2).")
    ap.add_argument("--art", default="ml/artifacts")
    ap.add_argument("--max-auc-drop", type=float, default=0.02)
    a = ap.parse_args()

    art = Path(a.art)
    ckpt = torch.load(art / "ae.pt", map_location="cpu", weights_only=False)
    manifest = json.loads((art / "ae_manifest.json").read_text())

    feat, _ = load_all(manifest["glob"])
    split = ts_split(feat, FEATURES)

    ae = build_ae(ckpt["arch"])
    ae.load_state_dict(ckpt["state_dict"])
    ae.eval()
    auc_f = roc_auc_score(split.yte, recon_error(ae, split.Xte))

    export = quantize_weights(ae)                    # ae now holds int8 weights
    auc_q = roc_auc_score(split.yte, recon_error(ae, split.Xte))
    drop = auc_f - auc_q

    np.savez(art / "ae_int8.npz", **export)
    print(f"[float] ROC-AUC={auc_f:.4f}")
    print(f"[int8 ] ROC-AUC={auc_q:.4f}  drop={drop:+.4f}  "
          f"(gate<= {a.max_auc_drop}) -> {'PASS' if drop <= a.max_auc_drop else 'FAIL'}")
    print(f"[save ] {art/'ae_int8.npz'}")
    if drop > a.max_auc_drop:
        raise SystemExit("parity gate FAILED — int8 AUC drop exceeds budget")


if __name__ == "__main__":
    main()