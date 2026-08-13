"""
predictor.py - time-to-failure regression (the PREDICT flagship).
From the heap-free slope, regress remaining TTF; score against the analytical
true_ttf already in the dataset (heap_free / drain_rate).
Produces: lead-time MAE/RMSE per class.
"""

# 1. fit(X, ttf)          -> small regressor on the faulty RAMP only
# 2. predict(model, X)    -> predicted ttf
# 3. lead_time_error(..)  -> MAE/RMSE vs analytical true_ttf
# NOTE: trim the post-OOM plateau (ttf pinned ~0.069) before fitting.