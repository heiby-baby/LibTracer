#!/bin/bash

./build/logdemon  "log_$(date +'%Y-%m-%d_%H-%M-%S')" 10 &
LD_PRELOAD=./build/injectLib.so ./build/test 
