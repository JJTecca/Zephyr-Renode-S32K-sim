"""
reporter.py - LLM incident reporter on the local GPU (agent E; EXPLAINS ONLY).
Local Qwen + RAG over pgvector: retrieve the incident + similar past ones, generate
a human-readable post-incident report. Off-vehicle, NEVER actuates. n8n-triggered.
"""

# 1. load_llm(model, device="cuda")  -> local Qwen on your GPU
# 2. retrieve(incident_id)           -> incident + k nearest (via cloud.api)
# 3. build_prompt(context)           -> grounded prompt (facts only, no actions)
# 4. generate(prompt)                -> report text
# 5. templated_report(ctx)           -> FALLBACK if the LLM is unavailable