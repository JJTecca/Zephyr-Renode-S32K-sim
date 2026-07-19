---
tags: [moc]
---
# Simulation & Fault Injection MOC
*Solves risk #1: fault data is a secret industrial asset — so I generate it reproducibly.*

## Core concepts
- [[Renode]] · [[SocketCAN vcan]] · [[Fault Taxonomy]] · [[Fault Injection]]

## Key questions
- Which fault classes for v1? (timing jitter · memory leak · thermal drift · packet loss / CAN error frames)
- How do I make each injected run fully reproducible (seed, schedule, config committed to git)?
- Claim to defend: *reproducible fault-data generation is itself a contribution.*

## Literature on this topic
```dataview
LIST FROM "10-Literature" WHERE contains(topics, "fault-injection") OR contains(topics, "simulation") SORT file.name
```
