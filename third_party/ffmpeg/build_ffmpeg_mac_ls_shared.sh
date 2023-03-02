#!/bin/sh

OPENSSL_DIR=$PWD/openssl
OPENSSL_BINARY_DIR=$OPENSSL_DIR/lib
OPENSSL_INCLUDE_DIR=$OPENSSL_DIR/include

rm -rf out/mac/*
make clean
./configure --enable-shared --disable-doc --disable-programs --disable-everything \
 --enable-encoder=aac,mp3 --enable-decoder=aac,mp3 --enable-muxer=aac,mp3 --enable-demuxer=aac,mp3 --enable-muxer=mp4 --enable-muxer=mov --enable-decoder=png,mjpeg --enable-demuxer=image2 \
 --enable-encoder=gif --enable-decoder=gif --enable-muxer=gif --enable-demuxer=gif \
 --disable-avdevice --disable-postproc --enable-avcodec --enable-avfilter --enable-openssl --disable-symver --disable-avresample --disable-audiotoolbox --disable-videotoolbox --disable-appkit --enable-bzlib \
 --enable-filter=abuffer,abuffersink,buffersink,buffer,atrim,showfreqs,aresample,showspectrumpic --disable-iconv --disable-securetransport --disable-avfoundation --disable-coreimage --disable-sdl2 --enable-zlib --enable-decoder=h264,mpeg4 --enable-encoder=h264,mpeg4 --enable-decoder=hevc --enable-demuxer=mov --enable-protocol=file,https --enable-protocol=file --disable-x86asm --enable-bsf=h264_mp4toannexb --enable-bsf=hevc_mp4toannexb --extra-cflags="-fno-stack-check -mmacosx-version-min=10.9" --prefix=out/mac\
 --extra-cflags=-I${OPENSSL_INCLUDE_DIR} \
 --extra-ldflags=-L${OPENSSL_BINARY_DIR};
make -j8;
make install;
make clean


install_name_tool -id @rpath/libavutil.56.dylib out/mac/lib/libavutil.56.dylib

install_name_tool -change out/mac/lib/libavutil.56.dylib @rpath/libavutil.56.dylib out/mac/lib/libavcodec.58.dylib
install_name_tool -id @rpath/libavcodec.58.dylib out/mac/lib/libavcodec.58.dylib

install_name_tool -change out/mac/lib/libavutil.56.dylib @rpath/libavutil.56.dylib out/mac/lib/libavformat.58.dylib
install_name_tool -change out/mac/lib/libavcodec.58.dylib @rpath/libavcodec.58.dylib out/mac/lib/libavformat.58.dylib
install_name_tool -id @rpath/libavformat.58.dylib out/mac/lib/libavformat.58.dylib

install_name_tool -change out/mac/lib/libavutil.56.dylib @rpath/libavutil.56.dylib out/mac/lib/libswresample.3.dylib
install_name_tool -id @rpath/libswresample.3.dylib out/mac/lib/libswresample.3.dylib

install_name_tool -change out/mac/lib/libavutil.56.dylib @rpath/libavutil.56.dylib out/mac/lib/libswscale.5.dylib
install_name_tool -id @rpath/libswscale.5.dylib out/mac/lib/libswscale.5.dylib

install_name_tool -change out/mac/lib/libavutil.56.dylib @rpath/libavutil.56.dylib out/mac/lib/libavfilter.7.dylib
install_name_tool -change out/mac/lib/libavcodec.58.dylib @rpath/libavcodec.58.dylib out/mac/lib/libavfilter.7.dylib
install_name_tool -change out/mac/lib/libswresample.3.dylib @rpath/libswresample.3.dylib out/mac/lib/libavfilter.7.dylib
install_name_tool -id @rpath/libavfilter.7.dylib out/mac/lib/libavfilter.7.dylib
