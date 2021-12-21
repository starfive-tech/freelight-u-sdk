#!/bin/sh

#./venc_test --codec=0 --picWidth=352 --picHeight=288 --coreIdx=0 --kbps=30 --lineBufInt=1 --loop-count=0 --lowLatencyMode=0 --packedFormat=0 --rotAngle=0 --mirDir=0 --secondary-axi=0 --frame-endian=31 --stream-endian=31 --source-endian=31 --srcFormat=0 --cfgFileName=./cfg/avc_inter_8b_02.cfg --output=./output/inter_8b_02.cfg.264 --input=./yuv/BasketballDrive_352x288_8b.yuv
./venc_test --codec=12 --cfgFileName=./cfg/hevc_fhd_inter_8b_11.cfg --output=./output/inter_8b_11.cfg.265 --input=./yuv/mix_1920x1080_8b_9frm.yuv
