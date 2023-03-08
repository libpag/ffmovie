#!/bin/bash -e
SOURCE_DIR=$(pwd)
BUILD_DIR=${SOURCE_DIR}/out/win
VENDOR_DIR=${SOURCE_DIR}/../../vendor
OPTIONS=" --disable-all --disable-everything --disable-autodetect --enable-avcodec --enable-avformat --enable-swresample --enable-swscale \
          --enable-decoder=aac,mp3,pcm_s16le,pcm_s16be,gif,pcm_mulaw  \
          --enable-demuxer=aac,mp3,wav,mov --enable-decoder=h264,mpeg4,hevc --enable-bsf=h264_mp4toannexb --enable-protocol=file \
          --enable-bsf=hevc_mp4toannexb \
          --disable-shared --enable-static --disable-doc"

if [[ ${VENDOR_BUILD_TYPE} == "Debug" ]]; then
  OPTIONS="${OPTIONS} --enable-debug"
fi

build_arch() {
  ./configure  --toolchain=msvc --target-os=${TARGET_OS} --arch=$ARCH $OPTIONS $EXTRA_OPTIONS \
    --prefix=$PREFIX_DIR
  make -j12
  make install
  make clean
}

cd ${SOURCE_DIR}

rm -rf $BUILD_DIR

echo "-------------- build arch x86 -----------------"

X264_INCLUDE=$VENDOR_DIR/libx264/win/include
X264_LIB=$VENDOR_DIR/libx264/win/x86

TARGET_OS="win32"
ARCH="x86"
PREFIX_DIR="${BUILD_DIR}/$ARCH"
build_arch

cd ${PREFIX_DIR}/lib
rename

cd ${SOURCE_DIR}

echo "-------------- build arch x64 -----------------"

X264_INCLUDE=$VENDOR_DIR/libx264/win/include
X264_LIB=$VENDOR_DIR/libx264/win/x64

TARGET_OS="win64"
ARCH="x86_64"
PREFIX_DIR="${BUILD_DIR}/x64"
build_arch

# copy all libraries.

cd ${PREFIX_DIR}/lib
rename

cp -r $BUILD_DIR/x64/include/. $BUILD_DIR/include