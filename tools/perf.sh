#/usr/bin/env bash
pushd ~/dev/Rack
perf record -g -o ./plugins/SIM/build/perf.data ./Rack -d
popd

perf script -i ./build/perf.data > ./build/out.perf
./tools/FlameGraph/stackcollapse-perf.pl ./build/out.perf > ./build/out.folded
./tools/FlameGraph/flamegraph.pl ./build/out.folded > ./build/flamegraph.svg
/snap/bin/brave ./build/flamegraph.svg

