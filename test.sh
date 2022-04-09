#!/usr/bin/env bash

echo " --- Running tests... "
cd build/
    cd test/
        ./tests "$@"
    cd ..
cd ..
