#!/usr/bin/env bash

echo " --- Building (with make)..."
		cd build
			make VERBOSE=1 --no-print-directory -j$(nproc)
		cd ..
echo " --- ... done"
