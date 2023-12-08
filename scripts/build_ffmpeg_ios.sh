#!/bin/bash -e
SOURCE_DIR=$(pwd)
IPHONEOS_DEPLOYMENT_TARGET=9.0
BUILD_DIR=${SOURCE_DIR}/out/ios
VENDOR_DIR=${SOURCE_DIR}/../../vendor

OPTIONS="--disable-all --disable-everything --disable-autodetect --enable-small \
         --enable-avcodec --enable-avformat --enable-swresample --enable-swscale \
         --enable-encoder=aac,mp3 --enable-decoder=aac,mp3,pcm_s16le,pcm_s16be,pcm_f32be,pcm_f32le --enable-muxer=aac,mp3 \
         --enable-demuxer=aac,mp3,wav --enable-muxer=mov --enable-muxer=mp4 \
         --enable-decoder=h264,mpeg4 --enable-encoder=h264,mpeg4 \
         --enable-decoder=hevc --enable-demuxer=mov --enable-protocol=file --enable-bsf=h264_mp4toannexb \
         --enable-bsf=hevc_mp4toannexb "

if [[ ${VENDOR_BUILD_TYPE} != "Debug" ]]; then
  OPTIONS="${OPTIONS} --disable-debug"
fi

build_arch() {
  ./configure --target-os=darwin --enable-cross-compile --arch=$ARCH --cc="$CC" $OPTIONS \
    --extra-cflags="-w $CFLAGS -arch $ARCH -fvisibility=hidden" --extra-ldflags="$LDFLAGS -arch $ARCH" --prefix=$BUILD_DIR/$OUT
  make -j12 >/dev/null
  make install >/dev/null
  make clean
}

rm -rf $BUILD_DIR
# build x86_64
CC="xcrun -sdk iphonesimulator clang"
ARCH="x86_64"
CFLAGS="-w -mios-simulator-version-min=${IPHONEOS_DEPLOYMENT_TARGET}  -arch $ARCH"
LDFLAGS="-arch $ARCH"
OUT=$ARCH
build_arch

# build armv7
CC="xcrun -sdk iphoneos clang"
ARCH="armv7"
CFLAGS="-w -mios-version-min=${IPHONEOS_DEPLOYMENT_TARGET} -arch $ARCH"
LDFLAGS="-arch $ARCH"
OUT=$ARCH
build_arch

# build arm64
CC="xcrun -sdk iphoneos clang"
ARCH="arm64"
CFLAGS="-w -mios-version-min=${IPHONEOS_DEPLOYMENT_TARGET}  -arch $ARCH"
LDFLAGS="-arch $ARCH"
OUT=$ARCH
build_arch

# build arm64-simulator
CC="xcrun -sdk iphonesimulator clang"
ARCH="arm64"
CFLAGS="-w -mios-simulator-version-min=${IPHONEOS_DEPLOYMENT_TARGET}  -arch $ARCH"
LDFLAGS="-arch $ARCH"
OUT="arm64-simulator"
build_arch

cp -r $BUILD_DIR/arm64/include/. $BUILD_DIR/include
