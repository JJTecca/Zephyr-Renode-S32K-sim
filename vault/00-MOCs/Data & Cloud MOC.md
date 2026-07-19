---
tags: [moc]
---
# Data & Cloud MOC

Where the heavy AI lives ([[ADR-002 — Heavy AI lives in the cloud]]): [[MQTT]] → [[Redpanda]] →
[[InfluxDB]]/[[TimescaleDB]] + [[pgvector]] → [[FastAPI]] → [[Grafana]] + React HITL panel, all on [[Railway]].
The cloud retrains, quantizes and ships models back via [[OTA update]]; it proposes recoveries but never
commands them ([[ADR-004 — Cloud proposes, K3 disposes]]).

Data generation: [[Renode]] + [[SocketCAN]]/[[vcan]] [[fault injection]] ([[ADR-006 — Fault data is generated, reproducibly]]).
Fleet direction (future work): [[federated learning]] — [[McMahan 2016 — FedAvg]], [[Beutel 2020 — Flower]].
Vehicle predictive-maintenance context: [[Loseto 2026 — Predictive Maintenance with Environmental Context]],
[[Weiss 2024 — Self-adaptive Anomaly Detection in Software-Defined Systems]].
SDV context: [[Liotou 2026 — Rise of the Software-Defined Vehicle]], [[SDV Security Survey 2025]],
[[Haeberle 2020 — Softwarization of Automotive E-E Architectures]], [[Laclau 2024 — Dynamic Service Orchestration for SDV]].
