#!/bin/bash

rm build/injectLib.so
rm build/logdemon
rm build/test
cd build
gcc -shared -fPIC -o injectLib.so ../src/injectLib.c -ldl
gcc ../src/logdemon.c -o logdemon -lrt
gcc ../src/test.c -o test