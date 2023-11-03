#!/bin/bash -e
SOURCE_DIR=$(pwd)
MACOSX_DEPLOYMENT_TARGET=10.13
BUILD_DIR=${SOURCE_DIR}/out/linux
VENDOR_DIR=${SOURCE_DIR}/../../vendor
OPTIONS="--disable-doc \
        --disable-programs \
        --disable-everything \
        --enable-encoder=aac,mp3 \
        --enable-decoder=aac,mp3,ac3,pcm_s16le,pcm_s16be,amrnb,amrmb,pcm_mulaw,pcm_u8 \
        --enable-muxer=aac,mp3 \
        --enable-demuxer=aac,mp3,wav \
        --enable-muxer=mp4 \
        --enable-muxer=mov \
        --enable-decoder=png,mjpeg,gif \
        --enable-demuxer=image2,gif  \
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
        --enable-encoder=h264,mpeg4 \
        --enable-libx264 \
        --enable-encoder=libx264 \
        --enable-protocol=file \
        --enable-x86asm \
        --enable-bsf=h264_mp4toannexb \
        --enable-bsf=hevc_mp4toannexb \
        --enable-libx264 \
        --enable-libopencore-amrwb \
        --enable-libopencore-amrnb \
        --enable-libvpx --enable-decoder=libvpx_vp8 --enable-decoder=libvpx_vp9 --enable-decoder=vp8 --enable-decoder=vp9 --enable-parser=vp8 --enable-parser=vp9 \
        --enable-demuxer=flv,matroska,webm_dash_manifest --enable-demuxer=ogg,avi --enable-decoder=vorbis,opus \
        --enable-encoder=libx264"

if [[ ${VENDOR_BUILD_TYPE} != "Debug" ]]; then
  OPTIONS="${OPTIONS} --disable-debug"
fi

X264_INCLUDE=$VENDOR_DIR/libx264/linux/include
X264_LIB=$VENDOR_DIR/libx264/linux/x64

VPX_INCLUDE=$VENDOR_DIR/libvpx/linux/include
VPX_LIB=$VENDOR_DIR/libvpx/linux/x64

rm -rf $BUILD_DIR
./configure $OPTIONS --extra-cflags="-I/usr/local/include -I$X264_INCLUDE -I$VPX_INCLUDE" --extra-libs=-ldl --extra-ldflags="-L/usr/local/lib -L$X264_LIB -L$VPX_LIB" --prefix=$BUILD_DIR/x64
make -j12 >/dev/null
make install >/dev/null
make clean

cp -r $BUILD_DIR/x64/include/. $BUILD_DIR/include
