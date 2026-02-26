# valhalla_tracer

Stdin/stdout trace_attributes processor. Replaces Docker-based Valhalla HTTP calls with a native binary linked against `libvalhalla_micro.a`.

## Build

```bash
cd ~/git/libs
make -f macOS.mk build ARCH=arm64
```

Binary installed to: `build/macOS/arm64/bin/valhalla_tracer`

## Usage

```
valhalla_tracer <config.json> <tzdata_path>
```

**Arguments:**
- `config.json` — Valhalla config file (same format as Docker service uses)
- `tzdata_path` — path to timezone data directory

**Example:**
```bash
valhalla_tracer /Volumes/4tb/osm/valhalla/config_native.json ~/git/glmap/Resources/framework/tzdata
```

## Protocol

One JSON request per line in (stdin), one JSON response per line out (stdout). Same request format as the HTTP `?json=` parameter.

| Stream | Content |
|--------|---------|
| stdin  | One JSON request per line |
| stdout | One JSON response per line (result or error) |
| stderr | Valhalla logs + structured status lines |

### Status lines (stderr, JSON)

```
{"status":"ready"}                                  # after init
{"status":"done","requests":4821,"errors":12}       # on exit
```

### Behavior

- Empty/whitespace lines on stdin: skipped
- Lines >1 MB: rejected with error JSON, processing continues
- EOF on stdin: clean exit 0
- SIGTERM/SIGINT: finishes current request, then exits 0
- Malformed JSON: returns Valhalla error response, processing continues

## Examples

**Single request:**
```bash
echo '{"shape":[{"lat":52.52,"lon":13.3889,"time":0},{"lat":52.5196,"lon":13.3889,"time":5}],"costing":"auto","shape_match":"map_snap"}' | \
  valhalla_tracer /Volumes/4tb/osm/valhalla/config_native.json ~/git/glmap/Resources/framework/tzdata
```

**Batch (pipe N requests):**
```bash
for i in $(seq 100); do cat request.json; done | \
  valhalla_tracer /Volumes/4tb/osm/valhalla/config_native.json ~/git/glmap/Resources/framework/tzdata \
  > results.jsonl 2>/dev/null
```

**From Go (exec.Command):**
```go
cmd := exec.Command("valhalla_tracer", configPath, tzdataPath)
cmd.Stdin = requestPipe   // write JSON lines
cmd.Stdout = responsePipe // read JSON lines
cmd.Stderr = os.Stderr    // Valhalla logs
cmd.Start()
```

## Error responses

Malformed input returns a Valhalla error object (not a crash):
```json
{"error_code":100,"error":"Failed to parse json request","status_code":400,"status":"Bad Request"}
```

The process continues accepting requests after any error.
