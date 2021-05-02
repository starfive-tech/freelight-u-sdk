#!/bin/bash

CORES=$(getconf _NPROCESSORS_ONLN)

make clean || true
make distclean || true

git submodule sync
git submodule update --recursive --init

make MKFS_VFAT="/sbin/mkfs.vfat" \
    MKFS_EXT4="/sbin/mkfs.ext4" \
    PARTPROBE="/sbin/partprobe" \
    SGDISK="/sbin/sgdisk" \
    -j${CORES}
