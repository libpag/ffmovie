#!/bin/sh

while getopts "t:" opt
do
	case "$opt" in
      	t ) parameterT="$OPTARG" ;;
	esac
done

option="--disable-doc \
		--disable-programs \
		--disable-everything \
		--enable-encoder=aac \
		--disable-avdevice \
		--disable-postproc \
		--enable-avcodec \
		--disable-symver \
		--disable-avresample \
		--disable-bzlib \
		--disable-iconv \
		--disable-securetransport \
		--disable-avfoundation \
		--disable-coreimage \
		--disable-sdl2 \
		--disable-zlib \
		--enable-decoder=h264 \
		--enable-decoder=hevc \
		--enable-demuxer=mov \
		--enable-protocol=file \
		--disable-x86asm \
		--enable-bsf=h264_mp4toannexb \
		--enable-bsf=hevc_mp4toannexb \
		--disable-avfilter \
    --enable-decoder=mp3 \
    --enable-decoder=aac \
    --enable-demuxer=mp3 \
    --enable-demuxer=aac \
    --toolchain=msvc \
    --enable-cross-compile \
		"

OutFileName="win_x86_64"

case $parameterT in
	x86 )
		option="$option \
				--target-os=win32 \
				--arch=x86 \
				"
    OutFileName="win_x86"
		;;
	x86_64 )
		option="$option \
				--target-os=win64 \
				--arch=x86_64 \
				"
    OutFileName="win_x86_64"
		;;
esac

rm -rf out/${OutFileName}
make clean

mkdir out/${OutFileName}

./configure ${option}--extra-cflags="-fno-stack-check" --prefix=out/${OutFileName};
make -j8;
make install;
make clean
