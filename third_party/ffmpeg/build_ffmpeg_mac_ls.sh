#!/bin/sh
rm -rf out/mac/*
make clean
./configure --disable-doc --disable-programs --disable-everything \
 --enable-encoder=aac,mp3 --enable-decoder=aac,mp3 --enable-muxer=aac,mp3 --enable-demuxer=aac,mp3 --enable-muxer=mp4 --enable-muxer=mov --enable-decoder=png,mjpeg --enable-demuxer=image2 \
 --disable-avdevice --disable-postproc --enable-avcodec --enable-avfilter --disable-symver --disable-avresample --disable-audiotoolbox --disable-videotoolbox --disable-appkit --enable-bzlib \
 --disable-iconv --disable-securetransport --disable-avfoundation --disable-coreimage --disable-sdl2 --enable-zlib --enable-libx264 --enable-encoder=libx264 --enable-gpl --enable-decoder=h264,mpeg4 --enable-encoder=h264,mpeg4 --enable-decoder=hevc \
 --enable-demuxer=mov --enable-protocol=file --enable-x86asm --enable-bsf=h264_mp4toannexb --enable-bsf=hevc_mp4toannexb --enable-videotoolbox --enable-pthreads --enable-encoder=h264_vaapi --enable-hwaccel=h264_videotoolbox --enable-encoder=h264_videotoolbox \
 --enable-hwaccel=hevc_videotoolbox --enable-hwaccel=mpeg4_videotoolbox --enable-debug --extra-cflags=-I/usr/local/include --extra-ldflags=-L/usr/local/lib \
 --extra-cflags="-fno-stack-check -mmacosx-version-min=10.9" --prefix=out/mac;
make -j8;
make install;
make clean
