#!/bin/sh

cd "$(dirname "$0")" || exit 1

# some environments have cmake v2 as 'cmake' and v3 as 'cmake3'
# check for cmake3 first then fallback to just cmake
[ -n "$B_CMAKE" ] || B_CMAKE=$(command -v cmake3)
[ -n "$B_CMAKE" ] || B_CMAKE=$(command -v cmake)
if [ -z "$B_CMAKE" ]; then
    echo "ERROR: CMake not in $PATH, cannot build! Please install CMake, or if this persists, file a bug report."
    exit 1
fi

B_BUILD_DIR="${B_BUILD_DIR:-build}"
B_BUILD_TYPE="${B_BUILD_TYPE:-Debug}"
B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=${B_BUILD_TYPE} ${B_CMAKE_FLAGS:-}"

# Check if system is macOS
if [ "$(uname)" = "Darwin" ]; then
    # Source the macOS environment script if it exists
    if [ -f ./macos_environment.sh ]; then
        . ./macos_environment.sh
    fi

    # Detect latest compatible SDK version
    MACOS_VERSION=$(sw_vers -productVersion)
    AVAILABLE_SDK_VERSIONS=$(xcodebuild -showsdks | grep macosx | awk '{print substr($NF, 7)}' | sort -r)
    for SDK_VERSION in $AVAILABLE_SDK_VERSIONS; do
        if [[ "$(printf '%s\n' "$SDK_VERSION" "$MACOS_VERSION" | sort -V | head -n1)" == "$SDK_VERSION" ]]; then
            MACOS_SDK_VERSION=$SDK_VERSION
            break
        fi
    done

    # Different paths for Intel and Apple Silicon
    case "$(uname -m)" in
        "x86_64")
            B_CMAKE_FLAGS="${B_CMAKE_FLAGS} -DCMAKE_OSX_SYSROOT=$(xcode-select --print-path)/SDKs/MacOSX.sdk -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOS_SDK_VERSION}"
            ;;
        *)
            B_CMAKE_FLAGS="${B_CMAKE_FLAGS} -DCMAKE_OSX_SYSROOT=$(xcode-select --print-path)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOS_SDK_VERSION}"
            ;;
    esac
fi

# Prefer ninja if available
if command -v ninja 2>/dev/null; then
    B_CMAKE_FLAGS="-GNinja ${B_CMAKE_FLAGS}"
fi

# Set default QT_VERSION to 6
QT_VERSION="${QT_VERSION:-6}"
if [ "$QT_VERSION" -ne 6 ] && [ "$QT_VERSION" -ne 5 ]; then
    echo "Invalid QT_VERSION. It must be either 5 or 6." >&2
    exit 1
fi

echo "Using Qt version $QT_VERSION"

# allow local customizations to build environment
[ -r ./build_env.sh ] && . ./build_env.sh

set -e

# Initialise Git submodules
git submodule update --init --recursive

# Clean
rm -rf ${B_BUILD_DIR}
mkdir ${B_BUILD_DIR}
cd ${B_BUILD_DIR}

# Build
echo "Starting Input Leap $B_BUILD_TYPE build in '${B_BUILD_DIR}'..."
"$B_CMAKE" $B_CMAKE_FLAGS ..
"$B_CMAKE" --build . --parallel
echo "Build completed successfully"
