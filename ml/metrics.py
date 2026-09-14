import argparse, re, statistics as st
from pathlib import Path

TICK_HZ = 10.0
STEM  = re.compile(r"(memory_leak|deadline_miss)_(?:(\d+)_)?seed(\d+)")
TELEM = re.compile(r"TELEM,(\d+),\d+,(\d+),(\d+),(\d+)")     # t, sig, seq, val
SCORE = re.compile(r"K3,score,seq=(\d+),score=[\d.]+,alarm=([01])")

def parse_k1(path):
    heap, lat_t, healed = [], {}, False
    for ln in path.read_text(errors="ignore").splitlines():
        m = TELEM.match(ln)
        if m:
            t, sig, seq, val = int(m[1]), int(m[2]), int(m[3]), int(m[4])
            if sig == 1:   heap.append((t, val))   # SIG_HEAP_FREE
            elif sig == 5: lat_t[seq] = t          # SIG_LOOP_LATENCY = the seq K3 scores on
        elif ln.startswith("K1,heal,restart"):
            healed = True
    return heap, lat_t, healed

def parse_k3(path):
    first_alarm, approved, guardrail  = None, False, 0
    for ln in path.read_text(errors="ignore").splitlines():
        m = SCORE.match(ln)
        if m and m[2] == "1" and first_alarm is None: first_alarm = int(m[1])
        elif ln.startswith("K3,supervisor,approve"): approved = True
        elif ln.startswith("K3,supervisor,block"):   guardrail += 1
    return first_alarm, approved, guardrail

def incident(stem, k1, k3):
    m = STEM.match(stem)
    cls  = m[1] if m else "unknown"
    rate = int(m[2]) if (m and m[2]) else 0
    heap, lat_t, healed_line = parse_k1(k1)
    alarm_seq, approved, guardrail = parse_k3(k3)

    baseline = max((v for _t, v in heap), default=0)
    det_t = lat_t.get(alarm_seq) if alarm_seq is not None else None

    lead = None                                # leak only: warning = free-at-detect / rate
    if cls == "memory_leak" and det_t is not None and rate:
        free_at = next((v for t, v in reversed(heap) if t <= det_t), baseline)
        lead = free_at / (rate * TICK_HZ)

    recovery_t = None                          # a draining heap only rises when restart lands
    if det_t is not None and healed_line:
        post = [(t, v) for t, v in heap if t >= det_t]
        for (t0, v0), (t1, v1) in zip(post, post[1:]):
            if v1 > v0: recovery_t = t1; break
    healed = cls == "memory_leak" and recovery_t is not None
    mttr   = (recovery_t - det_t) / 1000.0 if (healed and det_t is not None) else None

    return dict(stem=stem, cls=cls, detected=alarm_seq is not None, lead_s=lead,
                approved=approved, healed=healed, mttr_s=mttr, guardrail=guardrail)

def mean(xs):
    xs = [x for x in xs if x is not None]
    return round(st.mean(xs), 2) if xs else None

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--campaign", default="evidence/campaign")
    a = ap.parse_args()
    d = Path(a.campaign)
    incs = []
    for k1 in sorted(d.glob("*_k1.log")):
        stem = k1.name[:-7]                    # strip "_k1.log"
        k3 = d / f"{stem}_k3.log"
        if k3.exists(): incs.append(incident(stem, k1, k3))
    if not incs:
        print(f"no *_k1.log/*_k3.log pairs in {d}"); return
    leak = [i for i in incs if i["cls"] == "memory_leak"]
    print(f"incidents             : {len(incs)}  (leak={len(leak)})")
    print(f"detection rate        : {sum(i['detected'] for i in incs)}/{len(incs)}")
    print(f"mean lead time (leak) : {mean(i['lead_s'] for i in leak)} s")
    print(f"recovery success      : {sum(i['healed'] for i in leak)}/{len(leak)}")
    print(f"mean MTTR (leak)      : {mean(i['mttr_s'] for i in leak)} s")
    print(f"supervisor guardrail     : {sum(i['guardrail'] for i in incs)}")

if __name__ == "__main__":
    main()