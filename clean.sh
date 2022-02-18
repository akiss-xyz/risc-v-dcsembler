#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Cleaning..."
	cd build
		make clean --no-print-directory
	cd ..
echo " --- ... done."
