FFmpeg README
=============

FFmpeg is a collection of libraries and tools to process multimedia content
such as audio, video, subtitles and related metadata.

## Libraries

* `libavcodec` provides implementation of a wider range of codecs.
* `libavformat` implements streaming protocols, container formats and basic I/O access.
* `libavutil` includes hashers, decompressors and miscellaneous utility functions.
* `libavfilter` provides a mean to alter decoded Audio and Video through chain of filters.
* `libavdevice` provides an abstraction to access capture and playback devices.
* `libswresample` implements audio mixing and resampling routines.
* `libswscale` implements color conversion and scaling routines.

## Tools

* [ffmpeg](https://ffmpeg.org/ffmpeg.html) is a command line toolbox to
  manipulate, convert and stream multimedia content.
* [ffplay](https://ffmpeg.org/ffplay.html) is a minimalistic multimedia player.
* [ffprobe](https://ffmpeg.org/ffprobe.html) is a simple analysis tool to inspect
  multimedia content.
* Additional small tools such as `aviocat`, `ismindex` and `qt-faststart`.

## Documentation

The offline documentation is available in the **doc/** directory.

The online documentation is available in the main [website](https://ffmpeg.org)
and in the [wiki](https://trac.ffmpeg.org).

### Examples

Coding examples are available in the **doc/examples** directory.

## License

FFmpeg codebase is mainly LGPL-licensed with optional components licensed under
GPL. Please refer to the LICENSE file for detailed information.

## Contributing

Patches should be submitted to the ffmpeg-devel mailing list using
`git format-patch` or `git send-email`. Github pull requests should be
avoided because they are not part of our review process and will be ignored.

## Project construction
### mac:
./build_ffmpeg_mac.sh

### iOS:
./build_ffmpeg_iOS.sh

Support architecture: arm64 armv7 x86_64

### Android:
Need to modify the path of ndk

NDK_HOME=/Users/kevingpqi/Documents/05Android/SDK/ndk/android-ndk-r15c

then
./build_ffmpeg_android.sh

Support architecture: armeabi-v7a arm64-v8a

### Windows
SOURCES:
https://pracucci.com/compile-ffmpeg-on-windows-with-visual-studio-compiler.html
https://gist.github.com/sailfish009/8d6761474f87c074703e187a2bc90bbc
http://roxlu.com/2016/057/compiling-x264-on-windows-with-msvc

1、 Download "MSYS2 x86_64" from "http://msys2.github.io" and install into "C:\workspace\windows\msys64" 
2、 pacman -S make gcc diffutils mingw-w64-{i686,x86_64}-pkg-config mingw-w64-i686-nasm mingw-w64-i686-yasm
3、 Rename C:\workspace\windows\msys64\usr\bin\link.exe to C:\workspace\windows\msys64\usr\bin\link_orig.exe, in order to use MSVC link.exe
4、 Install VS2015 or VS2017
5、 Complie    
    ===== 32 BITS BEGIN
    * Run "VS2015 x86 Native Tools Command Prompt"
    
    * Inside the command prompt, run: 
    # C:\workspace\windows\msys64\msys2_shell.cmd -mingw32 -use-full-path
    ===== 32 BITS END
    
    ===== 64 BITS BEGIN
    * Run "VS2015 x64 Native Tools Command Prompt"
    
    * Inside the command prompt, run: 
    # C:\workspace\windows\msys64\msys2_shell.cmd -mingw64 -use-full-path
    ===== 64 BITS END
    
6、then 
    ./build_ffmpeg_win.sh -t x86
    ./build_ffmpeg_win.sh -t x86_64
    
Support architecture: x86 x86_64

Reference link：https://gist.github.com/RangelReale/3e6392289d8ba1a52b6e70cdd7e10282

### Output path
out/
