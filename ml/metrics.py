"""
metrics.py - the thesis metrics campaign.
Aggregate >=30 injected incidents into headline numbers; each traceable to a dated
run ID + injection seed.
Produces: MTTR, recovery rate, FP/hour, ROC-AUC, float-vs-int8 table, veto count.
"""

# 1. collect(incidents)     -> per-incident outcomes (TimescaleDB)
# 2. mttr / recovery_rate   -> healing effectiveness
# 3. fpr_per_hour           -> from normal-operation windows
# 4. availability_delta     -> uptime with vs without self-healing
# 5. report_table()         -> the defensible summary (-> thesis)