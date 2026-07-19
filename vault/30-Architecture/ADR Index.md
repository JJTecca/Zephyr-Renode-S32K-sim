---
tags: [moc]
---
# ADR Index
*Every non-obvious design decision gets an ADR. Future-you (and the committee) will ask "why did you do it this way?" — this folder is the answer.*

```dataview
TABLE status, date FROM "30-Architecture" WHERE contains(tags, "adr") SORT file.name
```

Candidates to write next (mirrored as pending in the platform repo `docs/adr/`):
- [ ] ADR-013 — RTOS choice: FreeRTOS vs Zephyr (decide July, never revisit)
- [ ] ADR-014 — Time-series DB: InfluxDB vs TimescaleDB (Timescale strongly favored)
- [ ] ADR-015 — GNN edge semantics: what IS an edge? (CAN flow vs service dependency vs shared resource)
- [ ] ADR-016 — MQTT topic hierarchy + QoS policy

## Platform mirror mapping (`docs/adr/` in the repo root uses its own numbering)
| Vault (authoritative) | Platform mirror |
|---|---|
| ADR-005 (litert-torch path) | docs/adr/ADR-005 (conversion) |
| ADR-011 (resolved taxonomy) | docs/adr/ADR-004 (taxonomy scope) |
| ADR-013 candidate (RTOS) | docs/adr/ADR-001 |
| ADR-014 candidate (DB) | docs/adr/ADR-002 |
| ADR-015 candidate (edges) | docs/adr/ADR-003 |
| ADR-016 candidate (MQTT) | docs/adr/ADR-006 |
| frozen telemetry schema | docs/adr/ADR-007 |
