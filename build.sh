#!/bin/bash

make clean || true
make distclean || true

git submodule sync
git submodule update --recursive --init

make -j4
