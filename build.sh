#!/bin/bash

mkdir ./build-sdk && cd build-sdk

repo init -u https://github.com/starfive-tech/freelight-u-sdk.git -b starfive -m tools/manifests/riscv-sdk.xml --no-clone-bundle --depth=1
repo sync --no-tags --no-clone-bundle --current-branch

CORES=$(getconf _NPROCESSORS_ONLN)

cd freelight-u-sdk/

make clean || true
make distclean || true

make MKFS_VFAT="/sbin/mkfs.vfat" \
    MKFS_EXT4="/sbin/mkfs.ext4" \
    PARTPROBE="/sbin/partprobe" \
    SGDISK="/sbin/sgdisk" \
    -j${CORES}
