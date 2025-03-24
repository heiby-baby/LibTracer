#!/bin/bash

rm -rf build
mkdir build
cd build

gcc -shared -fPIC -o memlib.so ../memlib.c -ldl
gcc ../test.c -o test
gcc ../io_demon.c -o io_demon