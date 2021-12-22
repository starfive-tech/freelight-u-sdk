#!/bin/bash
set -e

CORES=$(getconf _NPROCESSORS_ONLN)

git submodule sync
git submodule update --recursive --init

#make clean || true
#make distclean || true
rm -rf work/
make MKFS_VFAT="/sbin/mkfs.vfat" \
    MKFS_EXT4="/sbin/mkfs.ext4" \
    PARTPROBE="/sbin/partprobe" \
    SGDISK="/sbin/sgdisk" \
    -j${CORES}
make vpudriver-build
rm -rf work/buildroot_initramfs/images/rootfs.tar
make MKFS_VFAT="/sbin/mkfs.vfat" \
    MKFS_EXT4="/sbin/mkfs.ext4" \
    PARTPROBE="/sbin/partprobe" \
    SGDISK="/sbin/sgdisk" \
    -j${CORES}

