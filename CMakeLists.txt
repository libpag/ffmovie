cmake_minimum_required(VERSION 3.1)
project(ffmovie)

# Options for building ffmovie
if (EMSCRIPTEN)
    option(FFMOVIE_BUILD_STATIC "Build static lib" ON)
else ()
    option(FFMOVIE_BUILD_STATIC "Build static lib" OFF)
endif ()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

if (EMSCRIPTEN)
    set(ARCH wasm)
elseif (ANDROID OR IOS)
    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        if (SDK_NAME STREQUAL iphonesimulator)
            set(ARCH arm64-simulator)
        else ()
            set(ARCH arm64)
        endif ()
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(ARCH x64)
    else ()
        set(ARCH arm)
    endif ()
else ()
    if (MSVC)
        set(ARCH ${MSVC_C_ARCHITECTURE_ID})
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR $CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")
        set(ARCH x64)
    elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(ARCH arm64)
    else ()
        set(ARCH x86)
    endif ()
endif ()

if (EMSCRIPTEN)
    set(WEB TRUE)
    set(PLATFORM web)
elseif (ANDROID)
    set(PLATFORM android)
    set(ENV{NDK_HOME} ${ANDROID_NDK})
elseif (IOS)
    set(PLATFORM ios)
elseif (APPLE)
    set(MACOS TRUE)
    set(PLATFORM mac)
elseif (WIN32)
    set(PLATFORM win)
elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    set(LINUX TRUE)
    set(PLATFORM linux)
endif ()


# Sets the default build type to release.
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-Werror -Wall -Wextra -Weffc++ -pedantic -Werror=return-type)
endif ()

if (MSVC)
    add_compile_options("/utf-8")
endif (MSVC)

# Sets flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DFFMOVIE_DEBUG)
endif ()

# collects include directories.
set(FFMOVIE_INCLUDES ./ include src vendor/libpag/include vendor/ffmpeg/include vendor/libyuv/include vendor/parser/include)

# collects source files.
file(GLOB_RECURSE FFMOVIE_FILES src/*.*)

add_compile_options(-fvisibility=hidden)

file(GLOB FFMPEG_FILES vendor/ffmpeg/${PLATFORM}/${ARCH}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
list(APPEND FFMOVIE_VENDOR_LIBS ${FFMPEG_FILES})
file(GLOB LIBX264_FILES vendor/libx264/${PLATFORM}/${ARCH}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
list(APPEND FFMOVIE_VENDOR_LIBS ${LIBX264_FILES})
file(GLOB LIBYUV_FILES vendor/libyuv/${PLATFORM}/${ARCH}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
list(APPEND FFMOVIE_VENDOR_LIBS ${LIBYUV_FILES})

if (APPLE)

    add_compile_options(-stdlib=libc++)

    find_library(Z_LIB NAMES z)
    list(APPEND FFMOVIE_PLATFORM_SHARED_LIBS ${Z_LIB})
    # hide symbols.
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -w  -read_only_relocs suppress -exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/ios/ffmovie.lds")

elseif (ANDROID)

    # optimizes the output size
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/android/ffmovie/export.def")
    add_compile_options(-ffunction-sections -fdata-sections -Os -fno-exceptions -fno-rtti)
    find_library(LOG_LIB log)
    list(APPEND FFMOVIE_VENDOR_LIBS ${LOG_LIB})
    find_library(Z_LIB NAMES z)
    list(APPEND FFMOVIE_PLATFORM_SHARED_LIBS ${Z_LIB})
elseif (WIN32)
    add_compile_options("/utf-8")
    find_library(WS_LIB NAMES ws2_32)
    list(APPEND FFMOVIE_PLATFORM_SHARED_LIBS ${WS_LIB})
    find_library(BCRYPT_LIB NAMES bcrypt)
    list(APPEND FFMOVIE_PLATFORM_SHARED_LIBS ${BCRYPT_LIB})
endif ()

if (FFMOVIE_BUILD_STATIC)
    add_library(ffmovie STATIC ${FFMOVIE_FILES})
else ()
    add_library(ffmovie SHARED ${FFMOVIE_FILES})
endif ()
target_include_directories(ffmovie PUBLIC ${FFMOVIE_INCLUDES})
target_link_libraries(ffmovie ${FFMOVIE_VENDOR_LIBS} ${FFMOVIE_PLATFORM_SHARED_LIBS})