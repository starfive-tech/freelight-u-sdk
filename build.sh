#!/bin/bash

CORES=$(getconf _NPROCESSORS_ONLN)

make clean || true
make distclean || true

git submodule sync
git submodule update --recursive --init

make -j${CORES}
