#!/bin/sh

while getopts "t:" opt
do
    case "$opt" in
        t ) parameterT="$OPTARG" ;;
    esac
done

NDK_HOME=/Users/kevingpqi/Documents/05Android/sdk/ndk/android-ndk-r15c
PREFIX=out/android
HOST_PLATFORM=darwin-x86_64
rm -rf ${PREFIX}/*

COMMON_OPTIONS="\
                --target-os=android \
                --disable-debug \
                --disable-programs \
                --disable-doc \
                --disable-programs \
                --disable-everything \
                --enable-avcodec \
                --enable-avformat \
                --enable-swscale \
                --disable-symver \
                --disable-avresample \
                --disable-avfilter \
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
                --disable-x86asm \
                "

case $parameterT in
    h264 )
        COMMON_OPTIONS="$COMMON_OPTIONS \
                        --enable-decoder=h264 \
                        "
        ;;
    filemovie )
        COMMON_OPTIONS="$COMMON_OPTIONS \
                        --enable-decoder=h264 \
                        --enable-decoder=hevc \
                        --enable-demuxer=mov \
                        --enable-protocol=file \
                        --enable-bsf=h264_mp4toannexb \
                        --enable-bsf=hevc_mp4toannexb \
                        --enable-decoder=mp3 \
                        --enable-decoder=aac \
                        --enable-demuxer=mp3 \
                        --enable-demuxer=aac \
                        "
        ;;
    all ) 
        COMMON_OPTIONS="$COMMON_OPTIONS \
                        --enable-decoder=h264 \
                        --enable-decoder=hevc \
                        --enable-demuxer=mov \
                        --enable-protocol=file \
                        --disable-x86asm \
                        --enable-bsf=h264_mp4toannexb \
                        --enable-bsf=hevc_mp4toannexb \
                        --enable-decoder=mp3 \
                        --enable-decoder=aac \
                        --enable-demuxer=mp3 \
                        --enable-demuxer=aac \
                        "
        ;;
esac


build_all(){
    for version in armeabi-v7a arm64-v8a; do
        echo "======== > Start build $version"
        case ${version} in
        armeabi-v7a )
            ARCH="arm"
            CPU="armv7-a"
            CROSS_PREFIX="$NDK_HOME/toolchains/arm-linux-androideabi-4.9/prebuilt/$HOST_PLATFORM/bin/arm-linux-androideabi-"
            SYSROOT="$NDK_HOME/platforms/android-21/arch-arm/"
            EXTRA_CFLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=softfp -mvectorize-with-neon-quad"
            EXTRA_LDFLAGS="-Wl,--fix-cortex-a8"
        ;;
        arm64-v8a )
            ARCH="aarch64"
            CPU="armv8-a"
            CROSS_PREFIX="$NDK_HOME/toolchains/aarch64-linux-android-4.9/prebuilt/$HOST_PLATFORM/bin/aarch64-linux-android-"
            SYSROOT="$NDK_HOME/platforms/android-21/arch-arm64/"
            EXTRA_CFLAGS=""
            EXTRA_LDFLAGS=""
        ;;
        x86 )
            ARCH="x86"
            CPU="i686"
            CROSS_PREFIX="$NDK_HOME/toolchains/x86-4.9/prebuilt/$HOST_PLATFORM/bin/i686-linux-android-"
            SYSROOT="$NDK_HOME/platforms/android-21/arch-x86/"
            EXTRA_CFLAGS=""
            EXTRA_LDFLAGS=""
        ;;
        x86_64 )
            ARCH="x86_64"
            CPU="x86_64"
            CROSS_PREFIX="$NDK_HOME/toolchains/x86_64-4.9/prebuilt/$HOST_PLATFORM/bin/x86_64-linux-android-"
            SYSROOT="$NDK_HOME/platforms/android-21/arch-x86_64/"
            EXTRA_CFLAGS=""
            EXTRA_LDFLAGS=""
        ;;
        esac

        echo "-------- > Start clean workspace"
        make clean

        echo "-------- > Start config makefile"
        configuration="\
            --prefix=${PREFIX} \
            --libdir=${PREFIX}/libs/${version}
            --incdir=${PREFIX}/includes/${version} \
            --pkgconfigdir=${PREFIX}/pkgconfig/${version} \
            --arch=${ARCH} \
            --cpu=${CPU} \
            --cross-prefix=${CROSS_PREFIX} \
            --sysroot=${SYSROOT} \
            --extra-ldexeflags=-pie \
            ${COMMON_OPTIONS}
            "

        echo "-------- > Start config makefile with ${configuration}"
        ./configure_android ${configuration}

        echo "-------- > Start make ${version} with -j8"
        make j8

        echo "-------- > Start install ${version}"
        make install
        echo "++++++++ > make and install ${version} complete."

    done
}

echo "-------- Start --------"
build_all
make clean
echo "-------- End --------"
