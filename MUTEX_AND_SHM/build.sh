#!/bin/bash
rm -rf build
mkdir build
cd build

gcc ../reset.c -o reset -lrt -pthread
gcc ../increment.c -o increment -lrt -pthread
