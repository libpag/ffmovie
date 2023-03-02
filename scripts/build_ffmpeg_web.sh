#!/bin/bash -e
SOURCE_DIR=$(pwd)
MACOSX_DEPLOYMENT_TARGET=10.13
BUILD_DIR=${SOURCE_DIR}/out/web
VENDOR_DIR=${SOURCE_DIR}/../../vendor
OPTIONS="--disable-autodetect --disable-everything --disable-network --enable-small  \
         --enable-decoder=mp3 \
         --enable-demuxer=mp3 \
         --enable-muxer=mp3 --enable-protocol=file \
         --target-os=none --arch=x86_32 --cpu=generic --enable-cross-compile \
         --disable-asm --disable-inline-asm --disable-stripping \
         --disable-pthreads \
         --disable-programs --disable-doc --disable-debug --disable-runtime-cpudetect --disable-autodetect \
         --ar=emar --ranlib=emranlib --objcc=emcc --dep-cc=emcc --cc=emcc --cxx=em++ "
# --cc=emcc --cxx=em++ 

if [[ ${VENDOR_BUILD_TYPE} != "Debug" ]]; then
  OPTIONS="${OPTIONS} --disable-debug"
fi


rm -rf $BUILD_DIR
emconfigure ./configure $OPTIONS --prefix=$BUILD_DIR/wasm
emmake make
emmake make install
emmake make clean

cp -r $BUILD_DIR/wasm/include/. $BUILD_DIR/include