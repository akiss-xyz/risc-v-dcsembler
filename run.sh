#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Running..."
	./build/dcs-embler "$@"
echo " --- ... done."
