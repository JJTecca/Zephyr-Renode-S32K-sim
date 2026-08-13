"""
api.py - FastAPI read layer over the backbone.
Serve telemetry + incidents to Grafana, the React HITL panel, and n8n.
Read-only surface; proposals/reports flow down but never actuate the vehicle.
"""

# 1. GET /telemetry?ecu=&signal=&since=  -> series from TimescaleDB
# 2. GET /incidents                      -> recent incidents (+ ttf, action)
# 3. GET /incidents/{id}/similar         -> pgvector nearest (for RAG)
# 4. WS  /live                           -> push new telemetry/incidents