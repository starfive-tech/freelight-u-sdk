#!/bin/bash

CORES=$(getconf _NPROCESSORS_ONLN)

if [ -d ./build-sdk ] ; then
    rm -rf ./build-sdk
fi

mkdir ./build-sdk && cd build-sdk

repo init -u https://github.com/sarnold/freelight-u-sdk.git -b manifest-pr -m tools/manifests/riscv-sdk.xml --no-clone-bundle --depth=1
repo sync --no-tags --no-clone-bundle --current-branch

cd freelight-u-sdk/

make clean || true
make distclean || true

make MKFS_VFAT="/sbin/mkfs.vfat" \
    MKFS_EXT4="/sbin/mkfs.ext4" \
    PARTPROBE="/sbin/partprobe" \
    SGDISK="/sbin/sgdisk" \
    -j${CORES}
