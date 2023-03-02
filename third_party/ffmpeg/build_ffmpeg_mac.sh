#!/bin/sh

while getopts "t:" opt
do
	case "$opt" in
      	t ) parameterT="$OPTARG" ;;
	esac
done


rm -rf out/mac/*
make clean

option="--disable-doc \
		--disable-programs \
		--disable-everything \
		--enable-pthreads \
		--enable-encoder=aac \
		--disable-avdevice \
		--disable-postproc \
		--enable-avcodec \
		--disable-symver \
		--disable-avresample \
		--disable-audiotoolbox \
		--disable-videotoolbox \
		--disable-appkit \
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
		--enable-asm \
		--enable-x86asm \
		--enable-bsf=h264_mp4toannexb \
		--enable-bsf=hevc_mp4toannexb \
		--enable-videotoolbox --enable-pthreads --enable-hwaccel=h264_videotoolbox --enable-hwaccel=hevc_videotoolbox --enable-hwaccel=mpeg4_videotoolbox \
		"

case $parameterT in
	filemovie )
		option="$option \
				--disable-avfilter \
				--enable-decoder=mp3 \
				--enable-decoder=aac \
				--enable-demuxer=mp3 \
				--enable-demuxer=aac \
				"
		;;
	all ) 
		option="$option \
				--enable-decoder=mp3 \
				--enable-decoder=aac \
				--enable-demuxer=mp3 \
				--enable-demuxer=aac \
				--enable-avfilter \
				--enable-muxer=mp4 \
				--enable-muxer=mov \
				"
		;;
esac

./configure ${option}--extra-cflags="-fno-stack-check -mmacosx-version-min=10.9" --prefix=out/mac;

make -j8;
make install;
make clean
