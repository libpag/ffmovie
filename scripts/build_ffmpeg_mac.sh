#!/bin/bash -e
SOURCE_DIR=$(pwd)
MACOSX_DEPLOYMENT_TARGET=10.13
BUILD_DIR=${SOURCE_DIR}/out/mac
VENDOR_DIR=${SOURCE_DIR}/../../vendor
OPTIONS="--disable-all --disable-everything --disable-autodetect --enable-small \
         --enable-avcodec --enable-avformat --enable-swresample --enable-swscale \
         --enable-encoder=aac,mp3 --enable-decoder=aac,mp3,ac3,pcm_s16le,pcm_s16be,amrnb,amrmb,gif,pcm_mulaw --enable-muxer=aac,mp3 \
         --enable-demuxer=aac,mp3,wav,gif --enable-muxer=mov --enable-muxer=mp4 \
         --enable-decoder=h264,mpeg4 --enable-encoder=h264,mpeg4 \
         --enable-decoder=hevc --enable-demuxer=mov --enable-protocol=file --enable-bsf=h264_mp4toannexb \
         --enable-bsf=hevc_mp4toannexb --enable-zlib \
         --enable-demuxer=flv,matroska,webm_dash_manifest --enable-demuxer=ogg,avi --enable-decoder=vorbis,opus \
         --enable-version3  --enable-hwaccel=mpeg4_videotoolbox \
         --enable-gpl --enable-libx264 --enable-encoder=libx264 --enable-shared"

if [[ ${VENDOR_BUILD_TYPE} != "Debug" ]]; then
  OPTIONS="${OPTIONS} --disable-debug"
fi

build_arch() {
  ./configure --target-os=darwin --arch=$ARCH --cc="$CC" $OPTIONS $EXTRA_OPTIONS\
    --extra-cflags="$CFLAGS" --extra-ldflags="$LDFLAGS" \
    --prefix=$PREFIX_DIR
  make -j12
  make install
  make clean
}

rm -rf $BUILD_DIR
# build x86_64
X264_INCLUDE=$VENDOR_DIR/libx264/mac/include
X264_LIB=$VENDOR_DIR/libx264/mac/x64

# VPX_INCLUDE=$VENDOR_DIR/libvpx/mac/include
# VPX_LIB=$VENDOR_DIR/libvpx/mac/x64

EXTRA_OPTIONS=""
CC="xcrun -sdk macosx clang"
ARCH="x86_64"
CFLAGS="-w -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET -I$X264_INCLUDE -arch $ARCH"
LDFLAGS="-L$X264_LIB -arch $ARCH"
PREFIX_DIR="$BUILD_DIR/x64"
build_arch

# build arm64
X264_INCLUDE=$VENDOR_DIR/libx264/mac/include
X264_LIB=$VENDOR_DIR/libx264/mac/arm64

VPX_INCLUDE=$VENDOR_DIR/libvpx/mac/include
VPX_LIB=$VENDOR_DIR/libvpx/mac/arm64

CPU=`sysctl -n machdep.cpu.brand_string`
if [[ ${CPU} == *Intel* ]]; then
  EXTRA_OPTIONS="--enable-cross-compile"
fi
ARCH="arm64"
CFLAGS="-w -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET -fembed-bitcode -I$X264_INCLUDE -arch $ARCH"
LDFLAGS="-L$X264_LIB -arch $ARCH"
PREFIX_DIR="$BUILD_DIR/arm64"
build_arch

cp -r $BUILD_DIR/arm64/include/. $BUILD_DIR/include