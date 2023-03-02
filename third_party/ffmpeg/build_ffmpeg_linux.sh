#!/bin/bash
echo "-----begin-----"

BUILD_DIR=out/linux

if [ -e ${BUILD_DIR} ];then
    echo "remove old build dictionary"
    rm -r ${BUILD_DIR}
fi

mkdir ${BUILD_DIR}

echo `pwd`

WORKSPACE=$(pwd)

export CC=/usr/local/bin/clang
export CXX=/usr/local/bin/clang++

option="--disable-doc \
        --disable-programs \
        --disable-everything \
        --enable-encoder=aac,mp3 \
        --enable-decoder=aac,mp3 \
        --enable-muxer=aac,mp3 \
        --enable-demuxer=aac,mp3 \
        --enable-muxer=mp4 \
        --enable-muxer=mov \
        --enable-decoder=png,mjpeg \
        --enable-demuxer=image2  \
        --disable-avdevice \
        --disable-postproc \
        --enable-avcodec \
        --enable-avfilter \
        --disable-symver \
        --disable-avresample \
        --enable-bzlib \
        --disable-iconv \
        --disable-sdl2 \
        --enable-zlib \
        --enable-decoder=h264,mpeg4 \
        --enable-decoder=hevc \
        --enable-demuxer=mov \
        --enable-protocol=file \
        --enable-x86asm \
        --enable-bsf=h264_mp4toannexb \
        --enable-bsf=hevc_mp4toannexb \
        --extra-cflags=-I/usr/local/include \
        --extra-ldflags=-L/usr/local/lib
		"


while getopts "t:p:" opt
do
	case "$opt" in
		t ) parameterT="$OPTARG" ;;
	esac
done


case $parameterT in
	 x264 )
	    echo "---build x264----"
	    cd dependancy/x264
	    PKG_CONFIG_PATH=${WORKSPACE}/out/lib/pkgconfig
	    ./configure --prefix=${WORKSPACE}/out --enable-pic  bindir="$HOME/bin" --enable-static
	    make -j8
	    make install
	    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
	    echo "---build x264--finish--"
	    cd ${WORKSPACE}

		option="$option \
				--disable-vaapi \
                --enable-libx264 \
                --enable-gpl \
                --enable-encoder=libx264
				"
		;;
esac

echo "---build ffmpeg----"
./configure ${option} --prefix=${BUILD_DIR};

make -j8;
make install;
make clean
