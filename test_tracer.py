#!/usr/bin/env python3
# test_tracer.py — Load test for valhalla_tracer using real traces.
#
# Usage:
#   python3 test_tracer.py [total] [parallel] [port]
#
# Defaults: 5000 traces, 8 parallel clients, port 8002.
# Reads traces from /Volumes/4tb/traffic/tracks_20251027_170958.csv.zip.
# Reports throughput, error rate, and RSS every 500 requests.

import csv
import json
import subprocess
import sys
import time
import urllib.request
import concurrent.futures
import io

csv.field_size_limit(10 * 1024 * 1024)

TOTAL = int(sys.argv[1]) if len(sys.argv) > 1 else 5000
PARALLEL = int(sys.argv[2]) if len(sys.argv) > 2 else 8
PORT = int(sys.argv[3]) if len(sys.argv) > 3 else 8002
URL = f"http://localhost:{PORT}/"
TRACES_ZIP = "/Volumes/4tb/traffic/tracks_20251027_170958.csv.zip"
TRACES_CSV = "tracks_20251027_170958.csv"

def get_pid():
    try:
        return subprocess.check_output(
            f"lsof -ti:{PORT}", shell=True
        ).decode().strip().split('\n')[0]
    except:
        return None

def get_rss_mb():
    pid = get_pid()
    if not pid:
        return 0
    try:
        rss_kb = int(subprocess.check_output(
            ["ps", "-o", "rss=", "-p", pid]
        ).decode().strip())
        return rss_kb / 1024
    except:
        return 0

def get_footprint_mb():
    pid = get_pid()
    if not pid:
        return 0
    try:
        out = subprocess.check_output(
            ["vmmap", "-summary", pid], stderr=subprocess.DEVNULL
        ).decode()
        for line in out.split('\n'):
            if 'Physical footprint:' in line and 'peak' not in line:
                # parse "Physical footprint:         284.7M" or "1.2G"
                val = line.split(':')[1].strip()
                if val.endswith('G'):
                    return float(val[:-1]) * 1024
                elif val.endswith('M'):
                    return float(val[:-1])
                elif val.endswith('K'):
                    return float(val[:-1]) / 1024
    except:
        pass
    return 0

# Load polylines from zip.
proc = subprocess.Popen(
    ["unzip", "-p", TRACES_ZIP, TRACES_CSV],
    stdout=subprocess.PIPE, stderr=subprocess.DEVNULL
)
reader = csv.DictReader(io.TextIOWrapper(proc.stdout, encoding='utf-8'))
polylines = []
for row in reader:
    polylines.append(row['pointsPoly'])
    if len(polylines) >= TOTAL:
        break
proc.terminate()
print(f"Loaded {len(polylines)} polylines, sending to {URL} with {PARALLEL} clients", file=sys.stderr)

rss_start = get_rss_mb()
fp_start = get_footprint_mb()
print(f"RSS start: {rss_start:.0f} MB  footprint: {fp_start:.0f} MB", file=sys.stderr)

ok = 0
errors = 0
t0 = time.time()

def send_one(poly):
    body = json.dumps({
        "encoded_polyline": poly,
        "costing": "auto",
        "shape_match": "map_snap",
        "filters": {"attributes": ["edge.way_id", "edge.speed"], "action": "include"}
    }).encode()
    req = urllib.request.Request(URL, data=body, method='POST')
    req.add_header('Content-Type', 'application/json')
    try:
        resp = urllib.request.urlopen(req, timeout=60)
        code = resp.getcode()
        resp.read()
        return code
    except urllib.error.HTTPError as e:
        e.read()
        return e.code
    except:
        return -1

with concurrent.futures.ThreadPoolExecutor(max_workers=PARALLEL) as pool:
    futures = {pool.submit(send_one, p): i for i, p in enumerate(polylines)}
    for i, f in enumerate(concurrent.futures.as_completed(futures)):
        code = f.result()
        if code == 200:
            ok += 1
        else:
            errors += 1
        if (i + 1) % 500 == 0:
            elapsed = time.time() - t0
            rps = (i + 1) / elapsed
            rss = get_rss_mb()
            fp = get_footprint_mb()
            print(f"  {i+1}/{TOTAL}  ok={ok} err={errors}  {rps:.1f} req/s  RSS={rss:.0f} MB  footprint={fp:.0f} MB", file=sys.stderr)

elapsed = time.time() - t0
rss_end = get_rss_mb()
fp_end = get_footprint_mb()
print(f"\nDone: {ok} ok, {errors} errors in {elapsed:.1f}s ({len(polylines)/elapsed:.1f} req/s)", file=sys.stderr)
print(f"RSS: {rss_start:.0f} → {rss_end:.0f} MB  footprint: {fp_start:.0f} → {fp_end:.0f} MB", file=sys.stderr)
