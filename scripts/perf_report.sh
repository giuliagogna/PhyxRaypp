#!/bin/bash

set -e

echo "=== Xmake configuration (profile) and compilation ==="
xmake f -m profile
xmake

echo "=== Launching profiling via perf ==="
perf record --call-graph dwarf time xmake run PhyxRadpp render examples/shapes_zoo.txt 1.0 1.0 --algorithm pathtracing --dimensions 100 75 --pathtracer_params 5 5 3 --antialiasing 1 --output generated_images/perf_report_image.png

echo "=== Open report ==="
perf report > report.txt