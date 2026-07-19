---
tags: [concept]
aliases: [Message Passing]
---
# message passing

The core [[GNN]] idea: each node updates itself from its neighbours — **say it out loud: that IS
propagation.** Explaining it in one sentence to the committee beats any implementation detail
(Skills Atlas). Our use: 2 layers of ONE type (GCNConv or SAGEConv — not both) on the 5-node ECU
graph; more layers = over-smoothing. See [[Fault Propagation & GNN MOC]].
