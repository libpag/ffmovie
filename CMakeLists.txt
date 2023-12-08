cmake_minimum_required(VERSION 3.1)
project(ffmovie)

include(./third_party/vendor_tools/vendor.cmake)

# Options for building ffmovie
if (EMSCRIPTEN)
    option(FFMOVIE_BUILD_STATIC "Build static lib" ON)
else ()
    option(FFMOVIE_BUILD_STATIC "Build static lib" OFF)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_VERBOSE_MAKEFILE ON)

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
set(FFMOVIE_INCLUDES ./ include src vendor/libpag/include vendor/ffmpeg/include third_party/libyuv/include vendor/parser/include)

# collects source files.
file(GLOB_RECURSE FFMOVIE_FILES src/*.*)

add_compile_options(-fvisibility=hidden)

file(GLOB FFMPEG_FILES vendor/ffmpeg/${PLATFORM}/${ARCH}/*.a)
file(GLOB LIBAV_FORMAT vendor/ffmpeg/${PLATFORM}/${ARCH}/libavformat.a)
list(REMOVE_ITEM FFMPEG_FILES ${LIBAV_FORMAT})
list(PREPEND FFMPEG_FILES ${LIBAV_FORMAT})
list(APPEND FFMOVIE_LIBS ${FFMPEG_FILES})
file(GLOB LIBYUV_FILES third_party/out/libyuv/${PLATFORM}/${ARCH}/*${CMAKE_STATIC_LIBRARY_SUFFIX})
list(APPEND FFMOVIE_LIBS ${LIBYUV_FILES})

add_vendor_target(FFMOVIE STATIC_VENDORS libyuv)
list(APPEND FFMOVIE_LIBS ${FFMOVIE_VENDOR_STATIC_LIBRARIES})

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
    list(APPEND FFMOVIE_LIBS ${LOG_LIB})
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
    add_library(ffmovie STATIC ${FFMOVIE_VENDOR_TARGET} ${FFMOVIE_FILES})
else ()
    add_library(ffmovie SHARED ${FFMOVIE_VENDOR_TARGET} ${FFMOVIE_FILES})
endif ()
target_include_directories(ffmovie PUBLIC ${FFMOVIE_INCLUDES})
target_link_libraries(ffmovie ${FFMOVIE_LIBS} ${FFMOVIE_PLATFORM_SHARED_LIBS})

# 添加可执行文件 target
add_executable(FFMovieBin bin/main.cpp)
target_link_libraries(FFMovieBin ffmovie)


