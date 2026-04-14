# AI Import Pipeline Performance Report

## 1. Purpose

This report defines the repeatable validation method for `PIPE-010`.

Goals:

1. Measure how much overhead AI-assisted import adds relative to base import.
2. Separate cold-start cost from steady-state cached cost.
3. Identify fail-open fallback behavior under sustained import load.
4. Provide a stable template for later real benchmark fills.

---

## 2. Scope

Covered components:

- `AIImportOrchestrator`
- `AIAssetAnnotator`
- `ModelCacheManager`
- export whitelist planning when import metadata is generated

Out of scope for this revision:

- real model inference quality scoring
- GPU-specific backend tuning
- mesh and texture post-processing throughput

---

## 3. Test Environment

Fill this section before each run.

| Item | Value |
|---|---|
| Date | TBD |
| Commit | TBD |
| OS | TBD |
| CPU | TBD |
| GPU | TBD |
| RAM | TBD |
| Build Type | TBD |
| Backend | TBD |
| Model | TBD |
| Storage | TBD |

---

## 4. Workload Matrix

Use the following workload groups.

| Workload ID | Asset Type | Count | Size Profile | Expected AI Pass |
|---|---|---|---|---|
| W1 | small textures | 100 | 256px to 1K | annotation only |
| W2 | medium meshes | 50 | 10k to 100k verts | annotation only |
| W3 | mixed props | 200 | mixed | annotation only |
| W4 | repeated reimport | 200 | same asset set | cache hit validation |
| W5 | forced fallback | 200 | mixed | fail-open validation |

Notes:

- W1 and W2 isolate asset-shape differences.
- W4 validates cache reuse and sidecar hit rate.
- W5 disables runtime or forces request creation failure to verify degradation path.

---

## 5. Modes To Compare

For each workload, collect at least these modes.

| Mode | Description |
|---|---|
| M0 | base import only, AI disabled |
| M1 | orchestrator enabled, runtime unavailable, fail-open enabled |
| M2 | orchestrator enabled, annotation pass enabled, cold cache |
| M3 | orchestrator enabled, annotation pass enabled, warm cache |

Recommended optional modes:

- `M4`: fail-open disabled, runtime unavailable
- `M5`: annotation sidecar export planning enabled

---

## 6. Metrics

Record the following metrics.

| Metric | Description |
|---|---|
| total_import_time_ms | end-to-end batch duration |
| mean_import_time_ms | average per asset |
| p95_import_time_ms | long-tail latency |
| p99_import_time_ms | worst tail latency |
| fallback_ratio | fallback count / candidate imports |
| request_prepare_ratio | prepared requests / candidate imports |
| cache_hit_ratio | sidecar hits / cache lookups |
| cache_write_count | stored sidecars |
| runtime_unavailable_count | number of degraded imports |
| memory_peak_mb | peak resident memory |

Suggested secondary metrics:

- `prepared_requests`
- `fallback_imports`
- `stored_sidecars`
- `loaded_sidecars`

---

## 7. Execution Procedure

1. Reset editor import cache for cold-cache runs.
2. Run `M0` first to establish base import throughput.
3. Run `M1` to measure pure fail-open overhead with no runtime.
4. Run `M2` with a fresh cache directory.
5. Run `M3` immediately after `M2` without deleting sidecars.
6. Repeat each mode three times and report mean and max.

Control rules:

- Keep asset set identical between modes.
- Do not mix cold and warm cache data in one aggregate line.
- Record exact project settings used for import and cache policy.

---

## 8. Acceptance Gates

Use these initial gates until real production targets replace them.

| Gate | Target |
|---|---|
| Fail-open correctness | 100% of fallback imports preserve base import completion |
| Cold-cache overhead | documented, no silent regression |
| Warm-cache overhead | materially lower than cold-cache run |
| Cache observability | sidecar hit/miss counts available |
| Stability | no crash, deadlock, or leaked singleton state during repeated runs |

---

## 9. Result Table Template

| Workload | Mode | Run | Total ms | Mean ms | P95 ms | P99 ms | Fallback Ratio | Cache Hit Ratio | Peak MB | Notes |
|---|---|---|---|---|---|---|---|---|---|---|
| W1 | M0 | 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | |
| W1 | M1 | 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | |
| W1 | M2 | 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | |
| W1 | M3 | 1 | TBD | TBD | TBD | TBD | TBD | TBD | TBD | |

---

## 10. Interpretation Checklist

When results are filled, answer these questions:

1. Does enabling the orchestrator without runtime introduce only bounded overhead?
2. Does warm-cache import significantly reduce request preparation or repeated annotation cost?
3. Are fallback counts explained by policy and environment, not hidden failures?
4. Do p95 and p99 remain acceptable for editor reimport workflows?
5. Is there any sign that singleton-based services retain dirty state across repeated batches?

---

## 11. Current Status

This document is a benchmark protocol and report scaffold.

Current repository state:

- failure fallback behavior is validated by automated tests in `PIPE-009`
- no real benchmark numbers are recorded in this revision
