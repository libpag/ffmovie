# ffmovie

**ffmovie** is a cross-platform video decoder library based on ffmpeg, designed to provide efficient decoding capabilities for use in [libpag](https://github.com/libpag/libpag) and related projects.

## Dependency Management

Before building, make sure to initialize and update all required submodules:

```bash
git submodule update --init --recursive
```

## Build ffmpeg Libraries

Before building ffmovie, you must build the ffmpeg vendor libraries for your target platform.

### macOS

1. Ensure you have [Homebrew](https://brew.sh/) installed.
2. Install required build tools:
   ```bash
   brew install cmake yasm
   ```
3. From the root directory of the ffmovie project, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   This script will build and organize the ffmpeg static libraries for macOS (both x64 and arm64) and create Apple xcframeworks.

### iOS

1. Make sure you have Xcode command line tools installed.
2. From the root directory, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   This will invoke the necessary iOS build scripts (`scripts/build_ffmpeg_ios.sh`) and generate static libraries for all supported iOS architectures (arm64, arm64-simulator, x86_64).

### Android

1. Ensure you have the Android NDK installed and configured.
2. From the root directory, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   This will call `scripts/build_ffmpeg_android.sh` to build ffmpeg for Android architectures (arm, arm64, x86_64).

### Windows

1. Install Visual Studio with C++ development tools.
2. From the root directory, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   This will build the ffmpeg static libraries for Windows using MSVC toolchain.

### Linux

1. Install required packages (e.g., `build-essential`, `yasm`).
2. From the root directory, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   The script will build the ffmpeg libraries for Linux.

### WebAssembly

1. Install [Emscripten](https://emscripten.org/).
2. From the root directory, run:
   ```bash
   ./build_ffmpeg.sh
   ```
   This will invoke `scripts/build_ffmpeg_web.sh` and produce ffmpeg libraries for the web.

The built ffmpeg libraries will be placed in the `vendor/ffmpeg` directory.

## Build ffmovie

After building the ffmpeg libraries, you have two ways to build ffmovie:

### 1. Build with CMake

After ffmpeg libraries are built, you can open this project by CLion to build, or simply build it with the cmake command-line tool.

### 2. Build with the `build_ffmovie` Script

You can also use the executable script `build_ffmovie` located in the project root directory. This script is powered by [vendor_tools](https://github.com/libpag/vendor_tools) and provides a unified build interface for multiple platforms.

Usage:

```bash
./build_ffmovie -p <platform>
```

or

```bash
./build_ffmovie --platform <platform>
```

Supported platforms are: `win`, `mac`, `ios`, `linux`, `android`, `web`, `ohos`.

For example, to build for Android:

```bash
./build_ffmovie -p android
```

The script will sync dependencies and invoke the appropriate build process for the specified platform.

> For more details, see the documentation in the [vendor_tools](https://github.com/libpag/vendor_tools) repository.

## License

This project is licensed under the terms of the [Apache 2.0 License](LICENSE).

---
**For more details on build options or troubleshooting, refer to the respective scripts in the `scripts/` directory.**
