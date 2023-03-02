#!/bin/bash -e
SOURCE_DIR=$(pwd)
BUILD_DIR=${SOURCE_DIR}/out/android
VENDOR_DIR=${SOURCE_DIR}/../../vendor
OPTIONS="--disable-all --disable-everything --disable-autodetect --enable-small \
         --disable-shared --enable-static \
         --enable-avcodec --enable-avformat --enable-swresample --enable-swscale \
         --enable-encoder=aac,mp3 --enable-decoder=aac,mp3,pcm_s16le,pcm_s16be,pcm_f32be,pcm_f32le --enable-muxer=aac,mp3 \
         --enable-demuxer=aac,mp3,wav --enable-muxer=mov --enable-muxer=mp4 \
         --enable-decoder=h264,mpeg4 --enable-encoder=h264,mpeg4 \
         --enable-decoder=hevc --enable-demuxer=mov --enable-protocol=file --enable-bsf=h264_mp4toannexb \
         --enable-bsf=hevc_mp4toannexb --enable-jni --enable-libx264 --enable-encoder=libx264 \
         --enable-gpl"

if [[ ${VENDOR_BUILD_TYPE} != "Debug" ]]; then
  OPTIONS="${OPTIONS} --disable-debug"
fi

find_ndk() {
  for NDK in $NDK_HOME $NDK_PATH $ANDROID_NDK_HOME $ANDROID_NDK; do
    if [ -f "$NDK/ndk-build" ]; then
      echo $NDK
      return
    fi
  done
  ANDROID_HOME=$HOME/Library/Android/sdk
  if [ -f "$ANDROID_HOME/ndk-bundle/ndk-build" ]; then
    echo $ANDROID_HOME/ndk-bundle
    return
  fi

  if [ -d "$ANDROID_HOME/ndk" ]; then
    for file in $ANDROID_HOME/ndk/*; do
      if [ -f "$file/ndk-build" ]; then
        echo $file
        return
      fi
    done
  fi
}

NDK_HOME=$(find_ndk)
TOOLCHAIN=$NDK_HOME/toolchains/llvm/prebuilt/darwin-x86_64
SYSROOT=$TOOLCHAIN/sysroot

build_arch() {
  ./configure --target-os=android --enable-cross-compile --cc=$CC --arch=$ARCH --cpu=${CPU} $OPTIONS \
    --cross-prefix=${CROSS_PREFIX} --sysroot=${SYSROOT} --extra-cflags="$CFLAGS" --extra-ldflags="$LDFLAGS" --prefix=$BUILD_DIR/$ARCH
  make -j12
  make install
  make clean
}

rm -rf $BUILD_DIR
# build arm64
X264_INCLUDE=$VENDOR_DIR/libx264/android/include
X264_LIB=$VENDOR_DIR/libx264/android/arm64
ARCH="arm64"
CPU="armv8-a"
CROSS_PREFIX=$TOOLCHAIN/bin/aarch64-linux-android-
CC=$TOOLCHAIN/bin/aarch64-linux-android21-clang
CFLAGS="-w -I$X264_INCLUDE -arch $ARCH"
LDFLAGS="-L$X264_LIB -arch $ARCH"
build_arch

# build armv7
X264_INCLUDE=$VENDOR_DIR/libx264/android/include
X264_LIB=$VENDOR_DIR/libx264/android/arm
ARCH="arm"
CPU="armv7-a"
CROSS_PREFIX=$TOOLCHAIN/bin/arm-linux-androideabi-
CC=$TOOLCHAIN/bin/armv7a-linux-androideabi21-clang
CFLAGS="-w -I$X264_INCLUDE -arch $ARCH"
LDFLAGS="-L$X264_LIB -arch $ARCH"
build_arch

cp -r $BUILD_DIR/arm64/include/. $BUILD_DIR/include