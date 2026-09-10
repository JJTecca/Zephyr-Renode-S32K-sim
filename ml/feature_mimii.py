from __future__ import annotations

import argparse
import glob
import os
from pathlib import Path

import numpy as np
import pandas as pd
import librosa

REPO = Path(__file__).resolve().parents[1]


def read_config(path):
    cfg = dict(sample_rate=16000, n_mels=32, frame_ms=64, hop_ms=32, machine="valve", snr_db=6)
    for line in Path(path).read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if ":" in line:
            k, v = (s.strip() for s in line.split(":", 1))
            if k in cfg:
                try:
                    cfg[k] = int(v)
                except ValueError:
                    cfg[k] = v
    return cfg


def clip_to_bands(wav, sr, n_mels, n_fft, hop):
    m = librosa.feature.melspectrogram(y=wav, sr=sr, n_fft=n_fft, hop_length=hop, n_mels=n_mels)
    return librosa.power_to_db(m).T.astype(np.float32)   # [frames, n_mels]


def main():
    ap = argparse.ArgumentParser(description="MIMII wav -> per-clip mel-band CSVs for the acoustic AE.")
    ap.add_argument("--config", default=str(REPO / "sim/configs/acoustic_anomaly.yaml"))
    ap.add_argument("--src", required=True, help="MIMII root")
    ap.add_argument("--out", default=str(REPO / "datasets"))
    ap.add_argument("--limit", type=int, default=0, help="max clips per class (0 = all)")
    a = ap.parse_args()

    cfg = read_config(a.config)
    sr, n_mels = int(cfg["sample_rate"]), int(cfg["n_mels"])
    n_fft = int(sr * int(cfg["frame_ms"]) / 1000)
    hop = int(sr * int(cfg["hop_ms"]) / 1000)
    cols = [f"band_{i}" for i in range(n_mels)]
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)

    idx = 0
    for label in ("normal", "abnormal"):
        wavs = sorted(glob.glob(os.path.join(a.src, "**", label, "*.wav"), recursive=True))
        if a.limit:
            wavs = wavs[:a.limit]
        faulty = label == "abnormal"
        for w in wavs:
            y, _ = librosa.load(w, sr=sr, mono=True)
            bands = clip_to_bands(y, sr, n_mels, n_fft, hop)
            df = pd.DataFrame(bands, columns=cols)
            df.insert(0, "faulty", faulty)
            df.insert(0, "timestamp", np.arange(len(df)) * int(cfg["hop_ms"]))
            df.to_csv(out / f"acoustic_{cfg['machine']}_{label}_{idx:04d}.csv", index=False)
            idx += 1
    print(f"[features] wrote {idx} clip CSVs to {out}  (n_mels={n_mels}, sr={sr}, n_fft={n_fft}, hop={hop})")


if __name__ == "__main__":
    main()