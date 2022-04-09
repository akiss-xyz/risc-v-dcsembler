#!/usr/bin/env bash
set -euxo pipefail # Exit on command failure and print all commands as they are executed.

echo " --- Opening doxygen docs (with xdg-open) - make sure you built them with ./build-doxygen.sh..."
cd build/html
    xdg-open index.html
cd ../..
