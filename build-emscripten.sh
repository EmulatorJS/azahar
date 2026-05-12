#!/bin/bash
set -e

cd "$(dirname "$0")"

if [[ -z "$EMSCRIPTEN" ]]; then
    echo "Run this script via emmake. Ex: emmake $0"
    exit 1
fi

CLEAN=0
for arg in "$@"; do
    case $arg in
        --clean) CLEAN=1 ;;
        *) echo "Unknown option: $arg"; echo "Usage: emmake $0 [--clean]"; exit 1 ;;
    esac
done

BUILD_DIR=build-emscripten

if [[ "$CLEAN" = "1" ]]; then
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f CMakeCache.txt ]; then
    emcmake cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-pthread -msimd128" \
        -DCMAKE_C_FLAGS="-pthread -msimd128" \
        -DENABLE_LIBRETRO=ON \
        -DENABLE_QT=OFF \
        -DENABLE_SDL2=OFF \
        -DENABLE_TESTS=OFF \
        -DENABLE_CUBEB=OFF \
        -DENABLE_OPENAL=OFF \
        -DENABLE_VULKAN=OFF \
        -DENABLE_LIBUSB=OFF \
        -DENABLE_SCRIPTING=OFF \
        -DENABLE_WEB_SERVICE=OFF \
        -DENABLE_SOFTWARE_RENDERER=OFF \
        -DENABLE_GENERIC=ON
fi

emmake make -j"$(nproc)"

EXCLUDES="libCatch2.a libCatch2Main.a libCatch2WithMain.a libgtest.a libgtest_main.a libgmock.a libgmock_main.a"
OUTPUT=azahar_libretro_emscripten.a

{
    echo "create $OUTPUT"
    find . -name '*.a' -not -path './CMakeFiles/*' -not -name "$OUTPUT" | sort | while read -r a; do
        base="${a##*/}"
        skip=0
        for ex in $EXCLUDES; do
            [ "$base" = "$ex" ] && skip=1 && break
        done
        [ $skip -eq 0 ] && echo "addlib ${a#./}"
    done
    echo "save"
    echo "end"
} > merge.mri

emar -M < merge.mri

echo
echo "Built: $PWD/$OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
