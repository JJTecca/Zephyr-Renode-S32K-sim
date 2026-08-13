"""
gnn.py - cross-ECU fault-propagation GNN (advisory; PROPOSES ONLY).
Graph = K1 nodes + K3 + bus + services. Given graph state at t, predict which ECU
degrades next. MUST beat an MLP-on-concatenated-features baseline.
Produces: top-1/top-2 next-node accuracy vs the MLP.
"""

# 1. build_graph(episodes)  -> node features + edges (bus flow / shared resource)
# 2. GCN(nn.Module)         -> 2-layer graph conv, node-degradation head
# 3. MLP baseline           -> the bar to beat
# 4. train / eval           -> top-1/top-2 accuracy, both models
# 5. propose(state)         -> ranked next-to-degrade ECU (advisory JSON)