#/usr/bin/env bash
pushd ~/dev/Rack
# ./RackCatch2 --runtests='$1' -d
echo "pausing"
read
./RackCatch2 --runtests='$1' 2>&1",
popd