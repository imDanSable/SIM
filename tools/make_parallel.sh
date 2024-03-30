#!/bin/bash
time make -j$(nproc) "$@"
echo "make -j$(nproc) threads"