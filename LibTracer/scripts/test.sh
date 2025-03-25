#!/bin/bash

./build/logdemon ./build/log/${1} $2 &
LD_PRELOAD=./build/injectLib.so ./build/test