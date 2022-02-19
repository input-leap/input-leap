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
"$B_CMAKE" --build . --parallel || exit 1
echo "Build completed successfully"
