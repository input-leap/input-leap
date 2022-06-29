#!/bin/sh

cd "$(dirname "$0")" || exit 1

# some environments have cmake v2 as 'cmake' and v3 as 'cmake3'
# check for cmake3 first then fallback to just cmake

B_CMAKE=$(command -v cmake3 2>/dev/null)
if [ "$?" -eq 0 ]; then
    continue
else
    if command -v cmake 2>/dev/null; then
        B_CMAKE=$(command -v cmake)
    else
        echo "ERROR: CMake not in $PATH, cannot build! Please install CMake, or if this persists, file a bug report."
        exit 1
    fi
fi

CMAKE_VERSION="$("$B_CMAKE" --version)"
if [ "$(printf '%s\n' "3.12.0" "$currentver" | sort -V | head -n1)" = "3.12.0" ]; then
    CMAKE_HAS_PARALLEL=1
else
    CMAKE_HAS_PARALLEL=0
fi

B_BUILD_TYPE="${B_BUILD_TYPE:-Debug}"
B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=${B_BUILD_TYPE} ${B_CMAKE_FLAGS:-}"

if [ "$(uname)" = "Darwin" ]; then
    # macOS needs a little help, so we source this environment script to fix paths.
    . ./macos_environment.sh
    B_CMAKE_FLAGS="${B_CMAKE_FLAGS} -DCMAKE_OSX_SYSROOT=$(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -DCMAKE_OSX_DEPLOYMENT_TARGET=10.9"
fi

# Prefer ninja if available
if command -v ninja 2>/dev/null; then
    B_CMAKE_FLAGS="-GNinja ${B_CMAKE_FLAGS}"
fi

# allow local customizations to build environment
[ -r ./build_env.sh ] && . ./build_env.sh

# Initialise Git submodules
git submodule update --init --recursive

rm -rf build
mkdir build || exit 1
cd build || exit 1
echo "Starting Input Leap $B_BUILD_TYPE build..."
"$B_CMAKE" $B_CMAKE_FLAGS .. || exit 1

if [ "$CMAKE_HAS_PARALLEL" -eq "1" ]; then
    "$B_CMAKE" --build . --parallel || exit 1
else
    if command -v ninja 2>/dev/null; then
        ninja || exit 1
    else
        make || exit 1
    fi
fi

echo "Build completed successfully"
