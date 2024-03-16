#/usr/bin/env bash
cd ~/dev/Rack
perf record -g -o ~/dev/out.data ./Rack
cd ~/dev
perf script -i out.data > out.perf
./FlameGraph/stackcollapse-perf.pl out.perf > out.folded
./FlameGraph/flamegraph.pl out.folded > flamegraph.svg
/snap/bin/brave flamegraph.svg

