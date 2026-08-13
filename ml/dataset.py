"""
dataset.py — load labelled campaign CSVs into ML-ready feature tables.
Reads datasets/*.csv (ADR-007 schema: timestamp, signal, value, label, true_ttf),
reshapes and feature-engineers for the baseline detectors and the predictor.
Produces: X/y frames + time-series train/test splits (NEVER random).
"""

# 1. load_episode(csv)  -> long frame (one row per timestamp+signal)
# 2. pivot_wide(frame)  -> one row per timestamp: [heap_free, heap_used, ...]
# 3. features(frame)    -> rolling heap-free slope (30 s window), deltas
# 4. targets(frame)     -> y_detect (normal/faulty), y_ttf (regression)
# 5. ts_split(frames)   -> chronological split; scalers FIT ON TRAIN ONLY
# 6. load_all(glob)     -> stack episodes/seeds for a proper ROC