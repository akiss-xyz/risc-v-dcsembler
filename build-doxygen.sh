#!/usr/bin/env bash

echo " --- Building doxygen docs (with make)..."
		cd build
			make VERBOSE=1 --no-print-directory doxygen-docs
		cd ..
echo " --- ... done"
