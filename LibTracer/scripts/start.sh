#!/bin/bash

./build/io_demon 100 &
LD_PRELOAD=./build/memlib.so ./build/test > log.txt 2>&1
