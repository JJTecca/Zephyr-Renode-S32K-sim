from __future__ import annotations

import argparse
from pathlib import Path

import torch


def cf(v) -> str:
    s = f"{float(v):.8g}"
    if "." not in s and "e" not in s and "E" not in s:
        s += ".0"
    return s + "f"


def carr1(name: str, vals) -> str:
    body = ", ".join(cf(v) for v in vals)
    return f"static const float {name}[{len(vals)}] = {{{body}}};\n"


def carr2(name: str, mat) -> str:
    rows = ", ".join("{" + ", ".join(cf(v) for v in r) + "}" for r in mat)
    return f"static const float {name}[{len(mat)}][{len(mat[0])}] = {{{rows}}};\n"


def main() -> None:
    ap = argparse.ArgumentParser(description="Emit ae_model.h for the M7 firmware.")
    ap.add_argument("--pt", default="ml/artifacts/ae.pt")
    ap.add_argument("--out", default="firmware/common/ae_model.h")
    a = ap.parse_args()

    ck = torch.load(a.pt, map_location="cpu", weights_only=False)
    sd = ck["state_dict"]
    arch = ck["arch"]                                # [4, 8, 3, 8, 4]
    mu, sdv = ck["scaler_mu"], ck["scaler_sd"]
    thr = float(ck["threshold"])

    idx = [0, 2, 4, 6]                               # Sequential positions of the Linears
    W = [sd[f"{i}.weight"].numpy() for i in idx]     # each [out, in]
    B = [sd[f"{i}.bias"].numpy() for i in idx]

    L = ["#ifndef SDV_AE_MODEL_H\n#define SDV_AE_MODEL_H\n",
         "/*****************************************************************************\n",
         "  * File:        ae_model.h                                                   \n",
         "* Description:   static const generated vars => DO NOT EDIT BY HAND           \n",
         "* Layer:       firmware  (on-vehicle loop host)                               \n",
         "* Project:     Zephyr-Renode-S32K-sim -- SDV Fault-Prediction & Self-Healing  \n",
         "* Copyright (c) 2026 Maior Cristian-Alexandru                                 \n",
         "******************************************************************************/\n"]
    for k, d in enumerate(arch):
        L.append(f"#define AE_D{k} {d}\n")
    L.append(f"#define AE_THRESHOLD {thr:.8g}f\n")
    L.append(carr1("ae_mu", mu))
    L.append(carr1("ae_sd", sdv))
    for k in range(4):
        L.append(carr2(f"ae_w{k}", W[k]))
        L.append(carr1(f"ae_b{k}", B[k]))
    L.append("#endif /* SDV_AE_MODEL_H */\n")

    out = Path(a.out)
    out.write_text("".join(L))
    print(f"[export] {out}  arch={arch}  threshold={thr:.6g}")


if __name__ == "__main__":
    main()