#!/bin/sh

while getopts "t:" opt
do
    case "$opt" in
        t ) parameterT="$OPTARG" ;;
    esac
done

FAT="out/iOS"

SCRATCH="scratch"
# must be an absolute path
THIN=`pwd`/"thin"

rm -rf $FAT/*
rm -rf config.h

# absolute path to x264 library
#X264=`pwd`/fat-x264

#FDK_AAC=`pwd`/../fdk-aac-build-script-for-iOS/fdk-aac-ios


if [ "$X264" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-gpl --enable-libx264"
fi

if [ "$FDK_AAC" ]
then
	CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-libfdk-aac --enable-nonfree"
fi

# avresample
#CONFIGURE_FLAGS="$CONFIGURE_FLAGS --enable-avresample"

CONFIGURE_FLAGS="\
				--enable-cross-compile \
				--disable-debug \
				--disable-programs \
        --disable-doc \
        --disable-programs \
        --disable-everything \
        --disable-avdevice \
        --disable-postproc \
        --disable-avfilter \
        --enable-avformat \
        --enable-avcodec \
        --disable-avresample \
        --disable-audiotoolbox \
        --enable-videotoolbox \
        --disable-bzlib \
        --disable-securetransport \
        --disable-avfoundation \
        --disable-coreimage \
        --disable-sdl2 \
        --disable-zlib \
                "

case $parameterT in
  h264 )
		CONFIGURE_FLAGS="$CONFIGURE_FLAGS \
		                --enable-decoder=h264 \
		                "
		;;
	filemovie )
		CONFIGURE_FLAGS="$CONFIGURE_FLAGS \
		                --enable-demuxer=mov \
                    --enable-protocol=file \
                    --enable-bsf=h264_mp4toannexb \
                    --enable-bsf=hevc_mp4toannexb \
						        --enable-decoder=mp3 \
                    --enable-decoder=aac \
                    --enable-demuxer=mp3 \
                    --enable-demuxer=aac \
                    --disable-asm \
                    --disable-optimizations \
						"
		CONFIGURE_DECODER="--enable-decoder=h264 \
							         --enable-decoder=hevc \
							         "
		;;
	all )
		CONFIGURE_FLAGS="$CONFIGURE_FLAGS \
                    --enable-demuxer=mov \
                    --enable-protocol=file \
                    --enable-bsf=h264_mp4toannexb \
                    --enable-bsf=hevc_mp4toannexb \
						        --enable-decoder=mp3 \
                    --enable-decoder=aac \
                    --enable-demuxer=mp3 \
                    --enable-demuxer=aac \
                    --enable-decoder=h264 \
							      --enable-decoder=hevc \
							      --enable-asm \
                    --enable-optimizations \
						"
		;;
esac

ARCHS="arm64 armv7 x86_64"

COMPILE="y"
LIPO="y"

DEPLOYMENT_TARGET="8.0"

if [ "$COMPILE" ]
then
	if [ ! `which yasm` ]
	then
		echo 'Yasm not found'
		if [ ! `which brew` ]
		then
			echo 'Homebrew not found. Trying to install...'
                        ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" \
				|| exit 1
		fi
		echo 'Trying to install Yasm...'
		brew install yasm || exit 1
	fi
	if [ ! `which gas-preprocessor.pl` ]
	then
		echo 'gas-preprocessor.pl not found. Trying to install...'
		(curl -L https://github.com/libav/gas-preprocessor/raw/master/gas-preprocessor.pl \
			-o /usr/local/bin/gas-preprocessor.pl \
			&& chmod +x /usr/local/bin/gas-preprocessor.pl) \
			|| exit 1
	fi

	CWD=`pwd`
	for ARCH in $ARCHS
	do
		echo "building $ARCH..."
		mkdir -p "$SCRATCH/$ARCH"
		cd "$SCRATCH/$ARCH"

		CFLAGS="-arch $ARCH"
		if [ "$ARCH" = "i386" -o "$ARCH" = "x86_64" ]
		then
		  CONFIGURE_FLAGS="$CONFIGURE_FLAGS
							$CONFIGURE_DECODER
							"

		    PLATFORM="iPhoneSimulator"
		    CFLAGS="$CFLAGS -mios-simulator-version-min=$DEPLOYMENT_TARGET"
		else
		  if [ "$ARCH" = "armv7" ]
	  	then
		         CONFIGURE_FLAGS="$CONFIGURE_FLAGS \
							--disable-asm \
							--disable-optimizations \
							"
		        EXPORT="GASPP_FIX_XCODE5=1"
		  fi
		    PLATFORM="iPhoneOS"
		    CFLAGS="$CFLAGS -mios-version-min=$DEPLOYMENT_TARGET -fembed-bitcode"
		    if [ "$ARCH" = "arm64" ]
		    then
		        EXPORT="GASPP_FIX_XCODE5=1"
		    fi
		fi

		XCRUN_SDK=`echo $PLATFORM | tr '[:upper:]' '[:lower:]'`
		CC="xcrun -sdk $XCRUN_SDK clang"

		# force "configure" to use "gas-preprocessor.pl" (FFmpeg 3.3)
		if [ "$ARCH" = "arm64" ]
		then
		    AS="gas-preprocessor.pl -arch aarch64 -- $CC"
		else
		    AS="gas-preprocessor.pl -- $CC"
		fi

		CXXFLAGS="$CFLAGS"
		LDFLAGS="$CFLAGS"
		if [ "$X264" ]
		then
			CFLAGS="$CFLAGS -I$X264/include"
			LDFLAGS="$LDFLAGS -L$X264/lib"
		fi
		if [ "$FDK_AAC" ]
		then
			CFLAGS="$CFLAGS -I$FDK_AAC/include"
			LDFLAGS="$LDFLAGS -L$FDK_AAC/lib"
		fi
		

		TMPDIR=${TMPDIR/%\/} $CWD/configure \
		    --target-os=darwin \
		    --arch=$ARCH \
		    --cc="$CC" \
		    --as="$AS" \
		    $CONFIGURE_FLAGS \
		    --extra-cflags="$CFLAGS" \
		    --extra-ldflags="$LDFLAGS" \
		    --prefix="$THIN/$ARCH" \
		|| exit 1

		make -j8 install $EXPORT || exit 1
		cd $CWD
	done
fi

if [ "$LIPO" ]
then
	echo "building fat binaries..."
	mkdir -p $FAT/lib
	set - $ARCHS
	CWD=`pwd`
	cd $THIN/$1/lib
	for LIB in *.a
	do
		cd $CWD
		echo lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB 1>&2
		lipo -create `find $THIN -name $LIB` -output $FAT/lib/$LIB || exit 1
	done

	cd $CWD
	cp -rf $THIN/$1/include $FAT
fi
rm -rf $THIN
rm -rf $SCRATCH
make clean

echo Done
