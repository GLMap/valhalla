# valhalla_tracer

HTTP trace_attributes server. Native binary linked against `libvalhalla_micro.a`.

## Build

```bash
cd ~/git/libs
touch download/valhalla && make -f macOS.mk build ARCH=arm64
```

Binary installed to: `build/macOS/arm64/bin/valhalla_tracer`

## Usage

```
valhalla_tracer <config.json> <tzdata_path> <port> [<workers>]
```

**Arguments:**
- `config.json` — Valhalla config file (same format as Docker service uses). Must have `mjolnir.tile_extract` pointing to an existing `.tar` file.
- `tzdata_path` — path to timezone data directory
- `port` — TCP port to listen on, 1–65535 (localhost only)
- `workers` — number of worker threads, 1–128 (default: 8)

**Example:**
```bash
valhalla_tracer /Volumes/4tb/osm/valhalla/config_native.json ~/git/glmap/Resources/framework/tzdata 8002 10
```

## Quick start (for agents)

Step-by-step guide to start the tracer and verify it works.

### 1. Check if already running

```bash
lsof -ti:8002
```

If a PID is returned, the tracer is already running — skip to step 3. To restart, kill it first:

```bash
kill $(lsof -ti:8002)
```

### 2. Start the server

```bash
cd ~/git/libs
build/macOS/arm64/bin/valhalla_tracer \
  /Volumes/4tb/osm/valhalla/config_native.json \
  ~/git/glmap/Resources/framework/tzdata \
  8002 10 \
  2>/tmp/tracer_stderr.log &
```

Wait for readiness (takes ~5 seconds to load tile extract):

```bash
sleep 6 && tail -1 /tmp/tracer_stderr.log
```

Expected output:

```
{"status":"ready","port":8002,"workers":10}
```

If you don't see this, check `/tmp/tracer_stderr.log` for errors.

### 3. Verify with a single request

```bash
curl -s -X POST http://localhost:8002/ \
  -d '{"shape":[{"lat":52.52,"lon":13.3889,"time":0},{"lat":52.5196,"lon":13.3889,"time":5}],"costing":"auto","shape_match":"map_snap"}'
```

A successful response is a JSON object with `"edges"` array. Any HTTP 200 response means the server is working.

### 4. Load test (optional)

```bash
python3 ~/git/libs/valhalla/test_tracer.py [total] [parallel] [port]
```

Defaults: 5000 traces, 8 parallel clients, port 8002. Reads real traces from `/Volumes/4tb/traffic/tracks_20251027_170958.csv.zip`. Reports throughput, error rate, RSS, and physical footprint every 500 requests.

Example — 1000 traces, 20 clients:

```bash
python3 ~/git/libs/valhalla/test_tracer.py 1000 20 8002
```

Expected: ~81% success rate (19% are unmatchable traces), ~48 req/s with 10 workers.

### 5. Stop the server

```bash
kill $(lsof -ti:8002)
```

### Common problems

| Symptom | Cause | Fix |
|---------|-------|-----|
| `Tile extract could not be loaded` | Wrong `tile_extract` path in config | Check that `/Volumes/4tb/osm/valhalla/tiles.tar` exists |
| `bind: Address already in use` | Port 8002 occupied | `kill $(lsof -ti:8002)` then retry |
| No `{"status":"ready"...}` after 10s | Binary not found or crash | Check `build/macOS/arm64/bin/valhalla_tracer` exists; rebuild if needed |
| `connection refused` on curl | Server not ready yet | Wait longer, or check stderr log |

## Config notes

- `mjolnir.tile_extract` — path to the tile tar archive (e.g. `tiles.tar`). If wrong, Valhalla logs `Tile extract could not be loaded` and tile lookups fail.
- `mjolnir.data_processing.scan_tar` — set to `false` to skip enumerating tiles at startup (faster init, tiles loaded on demand). With `false`, the startup log shows `tile count: {}` which is normal.
- `mjolnir.traffic_extract` — set to `""` if no traffic data available (logs a warning, not an error).

## HTTP API

`POST` any path with a JSON body → trace_attributes response.

Same request format as the Valhalla HTTP `?json=` parameter.

### Status lines (stderr, JSON)

```
{"status":"ready","port":8002,"workers":4}       # after init
{"status":"done","requests":4821,"errors":12}     # on shutdown
```

### Behavior

- Binds to `127.0.0.1` (localhost only)
- One actor per worker thread, each with its own GraphReader + tile cache
- Underlying tile_extract mmap is shared across all workers
- `auto_cleanup=true` — each actor releases parsed tiles after every request
- Body size limit: 2 MB
- Empty body: returns 400
- Malformed JSON: returns Valhalla error, server continues
- SIGTERM/SIGINT: finishes in-flight requests, then exits 0

## Examples

**Single request:**
```bash
curl -s -X POST http://localhost:8002/ \
  -d '{"shape":[{"lat":52.52,"lon":13.3889,"time":0},{"lat":52.5196,"lon":13.3889,"time":5}],"costing":"auto","shape_match":"map_snap"}'
```

**Load test (100 sequential requests):**
```bash
for i in $(seq 100); do
  curl -s -X POST http://localhost:8002/ -d @request.json > /dev/null
done
```

## Error responses

Empty body:
```json
{"error":"empty body","error_code":400}
```

Malformed JSON:
```json
{"error_code":100,"error":"Failed to parse json request","status_code":400,"status":"Bad Request"}
```

The server continues accepting requests after any error.

## Memory

- Each worker creates its own GraphReader with an independent LRU tile cache
- `max_cache_size` applies per worker — total cache memory = `max_cache_size × workers`
- The tile extract mmap is shared (single mapping) across all workers
- `auto_cleanup=true` clears routing workspace after each request, but NOT the tile cache
- Tile cache grows as new regions are hit and plateaus when LRU eviction kicks in

## Load test baseline (2026-02-27)

**Config:** `config_native.json`
```
max_cache_size:             8 GB
use_lru_mem_cache:          true
lru_mem_cache_hard_control: true
max_concurrent_reader_users: 4
```

**Run:** 4 workers, 5000 real traces from `tracks_20251027_170958.csv.zip` (worldwide, encoded polyline precision 6), 8 parallel HTTP clients.

| Metric | Value |
|--------|-------|
| Requests | 5000 |
| OK | 4051 (81%) |
| Errors | 949 (19%) |
| Throughput | 48 req/s |
| Duration | 104s |
| RSS baseline (idle after init) | 338 MB |
| RSS after 5000 requests | ~16 GB |
| RSS in Activity Monitor | 20 GB |

**Problem:** `max_cache_size` is 8 GB per GraphReader. With 4 workers, the theoretical max is 32 GB. Real traces hit tiles worldwide, filling all caches. This is too much for a local tool.

### Test 2: max_cache_size=4 GB (2026-02-27)

| Metric | Value |
|--------|-------|
| Requests | 5000 |
| OK / Errors | 4051 / 949 |
| Throughput | 70 req/s |
| RSS after 5000 requests | ~25 GB |

### Test 3: max_cache_size=1 GB (2026-02-27)

| Metric | Value |
|--------|-------|
| Requests | 5000 |
| OK / Errors | 4051 / 949 |
| Throughput | 73 req/s |
| RSS start | 130 MB |
| RSS after 5000 requests | ~26 GB |

### Test 4: 1 worker, max_cache_size=1 GB (2026-02-27)

| Metric | Value |
|--------|-------|
| Requests | 5000 |
| OK / Errors | 4051 / 949 |
| Throughput | 22 req/s |
| RSS start | 84 MB |
| RSS after 5000 requests | ~26 GB |

**Same RSS with 1 worker as with 4.** Memory does not scale with worker count.

### Root cause (found via `heap --guessNonObjects` and `vmmap`)

**Primary: meili grid cache (heap).** `meili::CandidateGridQuery` maintains `grid_cache_` that lazily indexes all edges in every geographic bin touched by traces. Two caches exist (Loki + Thor workers). `ClearFullCache()` only clears when `cache.size() > meili.grid.cache_size`. Default `cache_size=100240` is far too high — after 500 worldwide requests, only ~6000 bins are cached, but they hold 22M+ edge allocations totaling ~3 GB per 500 requests. The threshold never triggers, so the cache grows without limit.

**Secondary: mmap pages (RSS).** The 94 GB tile tar is mmapped via `GraphReader::get_extract_instance()`. The OS pages in accessed regions as RSS, but these are clean file-backed pages reclaimable under pressure. Not a real memory problem.

### Test 5: meili.grid.cache_size=500, 1 worker (2026-02-28)

| Metric | Value |
|--------|-------|
| Requests | 5000 |
| OK / Errors | 4051 / 949 |
| Throughput | 14.6 req/s |
| RSS (ps) | 12.5 GB (fluctuating, peaked 20 GB) |
| **Physical footprint (vmmap)** | **285 MB** |
| **MALLOC allocated** | **232 MB** |
| mapped file resident | 11.1 GB (clean, file-backed, reclaimable) |

**Fix confirmed.** Heap dropped from 13.6 GB → 232 MB. RSS is dominated by mmap pages which the OS manages.

## Config changelog

| Date | Change | Reason |
|------|--------|--------|
| 2026-02-27 | `tile_extract`: `tiles_new.tar` → `tiles.tar` | File was renamed, old path caused load failure |
| 2026-02-27 | `max_cache_size`: 8 GB → 4 GB → 1 GB | Testing cache impact on RSS — no effect found |
| 2026-02-28 | `meili.grid.cache_size`: 100240 → 500 | Fix unbounded grid cache growth, heap 13.6 GB → 232 MB |
| 2026-02-28 | `max_concurrent_reader_users`: 4 → 14 | Match core count for tile read parallelism |

## Performance tuning (2026-02-28)

Machine: 14-core Apple Silicon, 48 GB RAM. Memory budget: 20 GB.
Test: 5000 worldwide traces, `2× workers` parallel HTTP clients.

| Workers | Clients | cache_size | req/s | Footprint | Duration (5k) |
|---------|---------|------------|-------|-----------|----------------|
| 1 | 1 | 500 | 14.6 | 285 MB | 343s |
| 4 | 8 | 500 | 35.3 | 1.0 GB | 142s |
| 8 | 16 | 500 | 45.4 | 1.9 GB | 110s |
| 8 | 16 | 2000 | 40.1 | 6.0 GB | 125s |
| **10** | **20** | **500** | **47.5** | **2.0 GB** | **105s** |
| 10 | 20 | 1000 | 46.2 | 3.3 GB | 108s |
| 12 | 24 | 500 | 49.1 | 2.2 GB | 102s |
| 14 | 28 | 500 | 51.2 | 2.7 GB | 98s |

**Recommended config:** 10 workers, cache_size=500. 47.5 req/s at 2.0 GB physical footprint.

Notes:
- Throughput is CPU-bound, scales sub-linearly with workers: 4w→8w +29%, 8w→14w +13%
- `cache_size=500` is optimal. Larger values (1000, 2000) increase memory without improving speed — worldwide traces have poor cache locality
- Warm mmap cache does not improve throughput (47.6 req/s warm vs 47.5 cold)
- RSS shows ~21 GB due to mmap page-in of the 94 GB tile tar, but physical footprint (actual memory) is only 2–3 GB
- `footprint` from `vmmap` is the real metric; RSS is misleading with mmapped files
- More clients than 2× workers gives no benefit
