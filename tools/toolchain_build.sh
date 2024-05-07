#!/bin/bash

# Define the command mapping
declare -A commands
commands=(["lin"]="plugin-build-lin-x64"
          ["win"]="plugin-build-win-x64"
          ["mac"]="plugin-build-mac-x64"
          ["arm"]="plugin-build-arm-x64"
          ["all"]="plugin-build"
          ["analyze"]="plugin-analyze"
          ["clean"]="plugin-clean")

# If an argument was provided, get the command from it
if [ -n "$1" ]; then
    command=${commands[$1]}
fi

# If no argument was provided or the argument was invalid, ask the user for one
while [ -z "$command" ]; do
    echo "Please provide one of: lin, win, mac, arm, all, analyze, clean."
    read input
    command=${commands[$input]}
done

# If the command is still empty, the argument was invalid
if [ -z "$command" ]; then
    echo "Invalid argument."
    exit 1
fi

# Run the command
cd ~/dev/rack-plugin-toolchain
make -j$(nproc) $command PLUGIN_DIR=~/dev/Rack251/plugins/SIM
# echo "make -j$(nproc) $command PLUGIN_DIR=~/dev/Rack/plugins/SIM"