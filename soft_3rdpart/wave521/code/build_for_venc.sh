#!/bin/bash
source set_env.sh

OUTPUT=venc_driver
DRI_OPT=${OUTPUT}/driver
DRI_SRC=./vdi/linux/driver
FIRMWARE=../firmware/chagall.bin

VENC_BIN=venc_test
#FFMPEG_VENC_BIN=ffmpeg_venc_test
MULT_VENC_BIN=multi_instance_test

CP="cp -rdvp"
RM="rm -rf"
MKDIR="mkdir -p"

$RM $DRI_OPT
$MKDIR $DRI_OPT

# build venc driver
make -f Wave5xxDriver.mak
$CP $DRI_SRC/*.ko $DRI_SRC/load.sh $DRI_SRC/unload.sh $DRI_OPT
make -f Wave5xxDriver.mak  clean

# multi_instance_test
make -f Wave5xxMultiV2.mak
$CP $MULT_VENC_BIN $OUTPUT
make -f Wave5xxMultiV2.mak clean

#build ffmpeg support
#make -f Wave5xxEncV2.mak USE_FFMPEG=yes
#$CP $FFMPEG_VENC_BIN $OUTPUT
#make -f Wave5xxEncV2.mak USE_FFMPEG=yes clean

# build Venc
make -f Wave5xxEncV2.mak
$CP $VENC_BIN $OUTPUT
make -f Wave5xxEncV2.mak clean


$CP cfg yuv stream script/* $OUTPUT
$MKDIR $OUTPUT/output
