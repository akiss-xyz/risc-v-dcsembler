#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Building doxygen docs (with make)..."
		cd build
			make VERBOSE=1 --no-print-directory doxygen-docs
		cd ..

echo " --- ... done"
