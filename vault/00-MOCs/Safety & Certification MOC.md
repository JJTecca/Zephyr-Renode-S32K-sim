---
tags: [moc]
---
# Safety & Certification MOC
*What makes a committee trust the word "self-healing": the deterministic supervisor and the standards story.*

## Core concepts
- [[Safety Cage Pattern]] · [[ASIL-D]] · [[ISO 26262]] · [[SOTIF (ISO 21448)]] · [[Fail-Operational vs Fail-Safe]]

## The thesis-defense answer bank
Committees *will* ask (per the challenges doc):
- "What exactly does self-healing mean?" → restart · migrate · degrade, nothing else, all gated by [[Fig 06 - Safety Boundary]]
- "How do you guarantee the AI can't cause harm?" → AI proposes, deterministic plain-C supervisor disposes ([[ADR Index]])
- "Isn't the AI stochastic?" → only certifiable, bounded-latency TinyML sits in the real-time path

## Literature on this topic
```dataview
LIST FROM "10-Literature" WHERE contains(topics, "safety") SORT file.name
```
