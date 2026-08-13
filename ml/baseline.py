"""
baseline.py - anomaly-detection baselines per fault class.

scikit-learn Isolation Forest + a small PyTorch denoising autoencoder, trained on
the NORMAL window only; score anomalies on held-out faulty data. Logs to W&B.
Produces: ROC-AUC + false-positive/hour per class (headline credibility metric).
"""

# 1. train_isoforest(X_normal)   -> fitted IF
# 2. DenoisingAE(nn.Module)      -> reconstruction error = anomaly score
# 3. train_ae(X_normal)          -> fitted AE (early stop on val)
# 4. score(model, X)             -> per-sample anomaly score
# 5. roc_and_fpr(y, scores)      -> ROC-AUC, FP/hour at threshold
# 6. run(cfg)                    -> per-class table -> W&B + reports/