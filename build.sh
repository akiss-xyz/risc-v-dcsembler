#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Building (with make)..."
		cd build
			make VERBOSE=1 --no-print-directory -j$(nproc)
		cd ..
echo " --- ... done"
