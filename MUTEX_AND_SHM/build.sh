#!/bin/bash

gcc reset.c -o reset -lrt -pthread
gcc increment.c -o increment -lrt -pthread
