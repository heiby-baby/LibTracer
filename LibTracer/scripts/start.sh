#!/bin/bash

LD_PRELOAD=./build/memlib.so ./build/test > log.txt 2>&1
