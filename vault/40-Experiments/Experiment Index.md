---
tags: [moc]
---
# Experiment Index

```dataview
TABLE status, result, topics FROM "40-Experiments" WHERE contains(tags, "experiment") SORT file.name DESC
```

## Rules
1. No experiment without a written hypothesis *first*.
2. Every run records: git commit, data version, seed, tracker link.
3. Failed experiments get notes too — they're thesis content ("we tried X, it failed because Y").
