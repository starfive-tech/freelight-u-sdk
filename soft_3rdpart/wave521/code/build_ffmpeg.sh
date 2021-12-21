#!/bin/sh
if [ $# == 1 ];then
	cmd=$1
else
	cmd=build
fi
PWD=`pwd`
FFMPEG=${PWD}/../../ffmpeg
FFMPEG_SRC=$FFMPEG
PREFIX=${PWD}/ffmpeg

#FFMPEG_INC=${FFMPEG}/include
#FFMPEG_LIB=${FFMPEG}/lib
#FFMPEG_BIN=${FFMPEG}/bin
#FFMPEG_SHARE=${FFMPEG}/share

FFMPEG_VERSION="4.2.2"
CPU_CORE=`cat /proc/cpuinfo |grep "processor"|wc -l`

if [ $cmd == "clean" ];then #clean
	cd $FFMPEG_SRC
	cd ffmpeg-${FFMPEG_VERSION}
	make uninstall
	make clean

elif [ $cmd == "distclean" ];then   #distclean
	cd $FFMPEG_SRC
	cd ffmpeg-${FFMPEG_VERSION}
	make uninstall
	make clean
	cd ..
	rm -rf ffmpeg-${FFMPEG_VERSION}
	#rm -rf $FFMPEG_INC $FFMPEG_LIB $FFMPEG_BIN $FFMPEG_SHARE $FFMPEG_SRC/ffmpeg-${FFMPEG_VERSION}
else     					#build
	cd $FFMPEG_SRC
	if [ ! -d "ffmpeg-${FFMPEG_VERSION}" ];then
		tar -xvf ffmpeg-${FFMPEG_VERSION}.tar.gz
	fi
	cd ffmpeg-${FFMPEG_VERSION}
	./config_ffmpeg_riscv.sh $PREFIX
	make -j${CPU_CORE}
	make install
fi
