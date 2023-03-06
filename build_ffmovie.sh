#!/bin/bash -e

PROJECT_NAME="FFMovie"
BUILD_CONFIG="release"
BUILD_APP_DIR=$PROJECT_NAME/ios/build

rm -rf "out"
node build_vendor ffmovie -p android
node build_vendor ffmovie -p linux
node build_vendor ffmovie -p mac
node build_vendor ffmovie -p ios
mkdir "out/ffmovie/ios/merge"
mkdir "out/ffmovie/ios/merge/ios"
mkdir "out/ffmovie/ios/merge/sim"

function merge() {
  mkdir "out/ffmovie/ios/merge/${arch}"
  node vendor_tools/merge vendor/ffmpeg/ios/${arch}/*.a vendor/libyuv/ios/${arch}/*.a -o out/ffmovie/ios/merge/${arch}/ffmovie-vendor.a
}

arch="arm"
merge
arch="arm64"
merge
arch="arm64-simulator"
merge
arch="x64"
merge

lipo -create out/ffmovie/ios/merge/arm/ffmovie-vendor.a  out/ffmovie/ios/merge/arm64/ffmovie-vendor.a -o out/ffmovie/ios/merge/ios/ffmovie-vendor.a
lipo -create out/ffmovie/ios/merge/x64/ffmovie-vendor.a  out/ffmovie/ios/merge/arm64-simulator/ffmovie-vendor.a -o out/ffmovie/ios/merge/sim/ffmovie-vendor.a

xcodebuild -create-xcframework -library out/ffmovie/ios/merge/ios/ffmovie-vendor.a -library out/ffmovie/ios/merge/sim/ffmovie-vendor.a -output out/ffmovie/ios/ffmovie-vendor.xcframework

cd ios

xcodebuild clean -project $PROJECT_NAME.xcodeproj -scheme $PROJECT_NAME  -configuration $BUILD_CONFIG

xcodebuild  -project $PROJECT_NAME.xcodeproj -scheme $PROJECT_NAME  -configuration $BUILD_CONFIG -sdk iphoneos -archs="arm64 armv7" SYMROOT=$BUILD_APP_DIR SKIP_INSTALL=NO BUILD_LIBRARIES_FOR_DISTRIBUTION=YES
    #判断编译结果
    if test $? -eq 0
    then
        echo "~~~~~~~~~~~~~~~~~~~ iphoneos  arm64 armv7编译成功~~~~~~~~~~~~~~~~~~~"
    else
        echo "~~~~~~~~~~~~~~~~~~~ iphoneos  arm64 armv7编译失败~~~~~~~~~~~~~~~~~~~"
    exit 1
    fi

strip -x FFMovie/ios/build/Release-iphoneos/FFMovie.framework/FFMovie

cp -a FFMovie/ios/build/Release-iphoneos/FFMovie.framework ../out/ffmovie/ios/merge/ios

xcodebuild clean -project $PROJECT_NAME.xcodeproj -scheme $PROJECT_NAME  -configuration $BUILD_CONFIG

xcodebuild  -project $PROJECT_NAME.xcodeproj -scheme $PROJECT_NAME  -configuration $BUILD_CONFIG -sdk iphonesimulator -arch x86_64 -arch arm64 SYMROOT=$BUILD_APP_DIR SKIP_INSTALL=NO BUILD_LIBRARIES_FOR_DISTRIBUTION=YES
    #判断编译结果
    if test $? -eq 0
    then
        echo "~~~~~~~~~~~~~~~~~~~ iphonesimulator 编译成功~~~~~~~~~~~~~~~~~~~"
    else
        echo "~~~~~~~~~~~~~~~~~~~ iphonesimulator 编译失败~~~~~~~~~~~~~~~~~~~"
    exit 1
    fi

strip -x FFMovie/ios/build/Release-iphonesimulator/FFMovie.framework/FFMovie

cp -a FFMovie/ios/build/Release-iphonesimulator/FFMovie.framework ../out/ffmovie/ios/merge/sim

cd ..

xcodebuild -create-xcframework -framework out/ffmovie/ios/merge/ios/FFMovie.framework -framework out/ffmovie/ios/merge/sim/FFMovie.framework -output out/ffmovie/ios/FFMovie.xcframework

rm -rf "out/ffmovie/ios/merge"

exit 0