#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Configuring... (you can also use configure.sh.)"
	ccmake -S . -B build/
echo " --- ... done. You can now build with build.sh."
