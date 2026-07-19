---
tags: [moc]
---
# Fault Propagation & GNN MOC
*The PREDICT stage — the headline contribution. How does a fault in zone-A propagate across the E/E topology?*

## Core concepts
- [[Graph Neural Network]] · [[Fault Propagation]] · [[ECU Dependency Graph]] · [[Message Passing]]

## Key questions
- How do I represent the vehicle topology (Fig 02) as a graph? Nodes = ECUs/services? Edges = CAN FD / Ethernet TSN links?
- What labels does the GNN train on — does Renode fault injection give me ground-truth propagation paths?
- Prediction horizon: how far ahead is a propagation prediction still actionable?

## Literature on this topic
```dataview
LIST FROM "10-Literature" WHERE contains(topics, "gnn") OR contains(topics, "fault-propagation") SORT file.name
```

## Experiments on this topic
```dataview
TABLE status, result FROM "40-Experiments" WHERE contains(topics, "gnn") SORT file.name DESC
```
