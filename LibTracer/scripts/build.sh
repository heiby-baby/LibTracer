#!/bin/bash

rm build/injectLib.so
rm build/logdemon
rm build/test
cd build
gcc -shared -fPIC -o injectLib.so ../injectLib.c -ldl
gcc ../logdemon.c -o logdemon -lrt
gcc ../test.c -o test