# Example Calc — every Python function, one worked example

*A single real example carried through every function in the pipeline, with the actual
numbers. Data-shaping stages use one captured K1 log (2103 `TELEM` lines, leak rate
128 → 1280 B/s); the ML stages use all 12 `datasets/memory_leak_*.csv`. The point is to
see the calculation behind each function, not just its name.*

**The thread we follow:** a `heap_free = 7976` tick → labelled `faulty, true_ttf 6.231`
→ features `[7976, −1360, 0, 0]` → detectors → the 135 trained weights + threshold 4.63
→ int8 → `ae_model.h` → the M7 raises the alarm.

---

## `run_campaign.py` — raw log → labelled CSV

| function | example in → out | the calculation |
|--|--|--|
| `coerce(v)` | `"128"→128`, `"1.5"→1.5`, `"k1"→"k1"` | try `int()`, then `float()`, else keep the string |
| `read_config(yaml)` | → `{leak_bytes_per_tick:128, tick_hz:10, …}` | parse `key: value` lines into a dict |
| `parse_rows(log)` | → 2103 tuples, first `(200,1,8112)` | regex-keep `TELEM,…` lines → `(t, sig, val)` |
| `find_inject_ms(rows)` | → `47589` | first row where `sig==heap_used` **and** `val>2` |
| `label(rows, 47589, 1280, …)` | writes the CSV | `faulty = t≥47589`; `true_ttf = value/1280` (e.g. `7976/1280 = 6.231`) |

## `dataset.py` — CSV → features

| function | example in → out | the calculation |
|--|--|--|
| `load_episode(csv,0)` | `(2103,5)→(2103,6)` | adds an `episode=0` column |
| `pivot_wide(df)` | `(2103,6)→(700,7)` | the 3 signal-rows of a tick become 3 columns of 1 row |
| `_rolling_slope(s,10,0.1)` | `[8112,8112,7976,7840,7704] → [nan, nan, −680, −906.7, −1020]` | `diff → [nan,0,−136,−136,−136]`; average the last ≤10 diffs, ÷0.1 → e.g. row 3: `mean(0,−136,−136)/0.1 = −906.7` |
| `add_features(wide)` | `(700,7)→(700,9)` | appends `heap_free_slope`, `loop_latency_slope` |
| `load_all(glob)` | 12 CSVs → one feature frame | `load_episode` each, concat, pivot, add_features |
| `ts_split(feat)` | → `Xtr_normal (282,4)`, `Xte (418,4)` | 7 files→train, keep normal; `mu`/`sd` = mean/std of those rows; `z=(x−mu)/sd` |

*(That `_rolling_slope` row is the warmup ramp: the 10-tick window is still filling with
`−136` diffs, so the slope grows toward its `−1360` plateau.)*

## `baseline.py` — DETECT scoreboard

| function | example in → out | the calculation |
|--|--|--|
| `threshold_from_train(s, .99)` | `[.001,.002,.0015,9e6,8e6] → 8.96e6` | the 99th-percentile of the train scores |
| `fpr_per_hour(y, s, .0018)` | → `12000` FP/hour | normals above thr = 1; hours = `3/10/3600`; `1 ÷ 8.3e-5` |
| `run_isoforest(split)` | → test AUC `0.513` | Isolation Forest anomaly score per row (blind to slow drift → ~coin-flip on the leak) |
| `run_autoencoder(split)` | → test AUC `1.000` | AE reconstruction-error per row (separates cleanly) |
| `report(name, split, s_tr, s_te)` | prints AUC + FP/hour | `roc_auc_score(y, s_te)` and `fpr_per_hour(...)` |

## `predictor.py` — PREDICT (leak only)

| function | example in → out | the calculation |
|--|--|--|
| `ramp_frame(feat)` | kept `730` of `5876` rows | keep only `faulty & heap_free_slope<0 & true_ttf>0.1` |
| `ttf_split(ramp)` | → `Xtr (300,2)`, `ytr (300,)` | `X=[heap_free, heap_free_slope]`, `y=true_ttf`, split by episode |
| `predict_analytic(X)` | `[[7976,−1280],[4000,−1280]] → [6.231, 3.125]` | `heap_free ÷ (−slope)` = seconds to empty |
| `fit(Xtr,ytr)` | → `coef=[0.0006, 0.0005], intercept=1.116` | least-squares line `ttf ≈ a·heap + b·slope + c` |
| `predict(model, X)` | X → predicted seconds | apply the fitted line |
| `lead_time_error(true,hat)` | `[6,3] vs [5.5,3.4] → mae 0.45` | `mean(|true−hat|)` and `sqrt(mean((true−hat)²))` |

## `train_ae.py` — BUILD the detector

| function | example in → out | the calculation |
|--|--|--|
| `build_ae(4)` | → `[Linear,ReLU,Linear,ReLU,Linear,ReLU,Linear]` | the 4→8→3→8→4 network (random weights) |
| `train(split,…)` | → trained network | 300 passes: add noise, rebuild, score, nudge every `w,b` |
| `recon_error(ae,X)` | train-normal → per-row error | per row: `mean((rebuild − row)²)` |
| `fpr_per_hour(scores, thr)` | → `0.0` FP/hour | count normals above `max×1.1`, ÷ hours |

**The two constants that come out of here:**
- `mu`/`sd` = mean/std of each column over all 1165 train-normal rows → `[8112,0,0.192,0.013]` / `[1,1,0.394,0.137]`.
- `threshold` = `max(recon over 1165 normal rows) × 1.10 = 4.2085 × 1.10 = 4.6293`.

## `quantize.py` — shrink to int8 (side branch)

| function | example in → out | the calculation |
|--|--|--|
| `build_ae` / `recon_error` | (reused) | — |
| `quantize_weights(ae)` | layer 0 → `scale 0.006892`, `int8[0]=[16,44,28,112]` | `scale = max(|W|)/127`; `q = round(W/scale)` |
| `main()` (parity) | `AUC 1.000 → 1.000, drop 0.000` | `AUC_float − AUC_int8 ≤ 0.02` |

## `export_model.py` — freeze to C

| function | example in → out | the calculation |
|--|--|--|
| `cf(v)` | `8112.0→"8112.0f"`, `0.107→"0.10706355f"` | format float, ensure a decimal point, add `f` |
| `carr1("ae_b",[.31,.26])` | → `static const float ae_b[2] = {0.31f, 0.26f};` | join values into a 1-D C array |
| `carr2("ae_w",[[1,2],[3,4]])` | → `static const float ae_w[2][2] = {{1.0f, 2.0f}, {3.0f, 4.0f}};` | join rows into a 2-D C array |
| `main()` | writes `firmware/common/ae_model.h` | copies `w, b, mu, sd, threshold` verbatim into C |

---

## The one-example thread, end to end

```
heap_free = 7976  tick
  run_campaign   → label faulty, true_ttf = 7976/1280 = 6.231
  dataset        → features [7976, −1360, 0, 0]  → z-score with mu,sd
  baseline       → DETECT: AE AUC 1.000  (IsoForest 0.513)
  predictor      → PREDICT: analytic 7976/1280 = 6.231 s ; MAE in seconds
  train_ae       → learn 135 weights (loss 0.55 → 0.03) + threshold 4.63
  quantize       → int8 parity: AUC drop 0.000
  export_model   → ae_model.h : w, b, mu, sd, threshold as C consts
  main.c (M7)    → slope → z-score → dense×4 → mean((o−z)²) = huge → alarm=1
```

**In one line:** the data-shaping functions turn raw UART into a 4-number feature row;
`train_ae` distils 1165 normal rows into 135 weights + one threshold; `quantize`/`export`
freeze them into C; the M7 replays the same arithmetic on one row at a time.
