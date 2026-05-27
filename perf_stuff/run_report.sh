#!/bin/bash

set -e

echo "=== Xmake configuration (profile) and compilation ==="
xmake f -m profile
xmake

echo "=== Launching profiling via perf ==="
perf record --call-graph dwarf xmake run PhyxRadpp demo 1 1 ./perf_stuff/perf_image --algorithm pathtracing --antialiasing 4 --dimensions 100 100 --pathtracer_params 4 4 3

echo "=== Open report ==="
perf report > report.txt