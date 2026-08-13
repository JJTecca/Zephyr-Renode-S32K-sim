"""
ingest.py - vehicle -> cloud telemetry ingestion.
MQTT (vehicle/{id}/ecu/{node}/signal/{name}) -> Redpanda -> TimescaleDB (telemetry)
+ pgvector (incident embeddings). Transport/storage only — NEVER actuates.
"""

# 1. mqtt_subscribe(broker)   -> yield telemetry frames
# 2. to_redpanda(frames)      -> produce onto the topic
# 3. sink_timescale(frames)   -> hypertable insert (QoS 0 telemetry)
# 4. sink_incident(pgvector)  -> embed + store incidents (QoS 1)