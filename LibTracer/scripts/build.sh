#!/bin/bash

rm -rf build
mkdir build
cd build

gcc -shared -fPIC -o memlib.so ../memlib.c -ldl
gcc ../test.c -o test