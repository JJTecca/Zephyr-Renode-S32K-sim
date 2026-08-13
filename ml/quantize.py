"""
quantize.py — float32 -> int8 for the on-MCU autoencoder (THE VERTICAL SLICE).
int8 post-training quantization w/ calibration set; export via litert-torch to
LiteRT-Micro; parity check (float vs int8, AUC drop < 2 pts = defensible).
Produces: int8 model artifact + float-vs-int8 size/latency/AUC table.
"""

# 1. calibration_set(X_normal)      -> representative PTQ samples
# 2. quantize_ptq(ae, calib)        -> int8 model
# 3. parity_check(float, int8, X)   -> max abs diff, AUC delta (MUST pass)
# 4. export_ltm(int8)               -> LiteRT-Micro artifact for the M7 firmware
# 5. size_latency_table(...)        -> tensor-arena KB, indicative ms