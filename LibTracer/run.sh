#!/bin/bash

./scripts/build.sh

./scripts/test.sh "${1}_$(date +'%Y-%m-%d_%H-%M-%S')" $2
