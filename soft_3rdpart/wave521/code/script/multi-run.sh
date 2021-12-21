#!/bin/sh

#./venc_test --codec=0 --picWidth=352 --picHeight=288 --coreIdx=0 --kbps=30 --lineBufInt=1 --loop-count=0 --lowLatencyMode=0 --packedFormat=0 --rotAngle=0 --mirDir=0 --secondary-axi=0 --frame-endian=31 --stream-endian=31 --source-endian=31 --srcFormat=0 --cfgFileName=./cfg/avc_inter_8b_02.cfg --output=./output/inter_8b_02.cfg.264 --input=./yuv/BasketballDrive_352x288_8b.yuv
#./multi_instance_test --codec=0 --cfgFileName=./cfg/avc_inter_8b_02.cfg --output=./output/inter_8b_02.cfg.264 --input=./yuv/BasketballDrive_352x288_8b.yuv


./multi_instance_test -e 1,1,1 --instance-num=3 --codec=12,0,12 --output=./output/bg_8b_01.cfg.265,./output/inter_8b_02.cfg.264,./output/inter_8b_11.cfg.265 --input=./cfg/hevc_bg_8b_01.cfg,./cfg/avc_inter_8b_02.cfg,./cfg/hevc_fhd_inter_8b_11.cfg