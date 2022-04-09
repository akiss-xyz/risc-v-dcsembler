#!/usr/bin/env bash

echo " --- Running..."
cd sandbox/
	../build/dcsembler "$@"
cd ..
echo " --- ... done."
