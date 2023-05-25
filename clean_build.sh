#!/bin/sh
set -e

# Change to the directory of the script
cd "$(dirname "$0")" || exit 1

# Set default values for the build environment
: "${B_CMAKE:=$(command -v cmake3)}"
: "${B_CMAKE:=$(command -v cmake)}"
: "${B_BUILD_DIR:=build}"
: "${B_BUILD_TYPE:=Debug}"

# Check if CMake is installed
if [ -z "$B_CMAKE" ]; then
    echo "ERROR: CMake not in \$PATH, cannot build! Please install CMake, or if this persists, file a bug report."
    exit 1
fi

# Check if ninja is installed and modify the CMake flags if it is
if command -v ninja >/dev/null; then
    B_CMAKE_FLAGS="-GNinja ${B_CMAKE_FLAGS}"
fi

# Default flags for CMake
B_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=${B_BUILD_TYPE} ${B_CMAKE_FLAGS}"

# Check if system is MacOS
if [ "$(uname)" = "Darwin" ]; then
    # Source the macOS environment script if it exists
    if [ -f ./macos_environment.sh ]; then
        . ./macos_environment.sh
    fi

    # Detect latest compatible SDK version
    MACOS_VERSION=$(sw_vers -productVersion)
    AVAILABLE_SDK_VERSIONS=$(xcodebuild -showsdks | grep macosx | awk '{print substr($NF, 7)}' | sort -r)
    for SDK_VERSION in $AVAILABLE_SDK_VERSIONS
    do
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

# Allow local customizations to build environment
if [ -f ./build_env.sh ]; then
    . ./build_env.sh
fi

# Check Git submodules
if ! git submodule status | egrep -q '^ '; then
    echo "Initializing Git submodules"
    git submodule update --init --recursive
else
    echo "Git submodules already initialized"
fi

# Build
echo "Starting build in '${B_BUILD_DIR}'..."
rm -rf "${B_BUILD_DIR}"
mkdir "${B_BUILD_DIR}"
cd "${B_BUILD_DIR}"
"$B_CMAKE" "$B_CMAKE_FLAGS" ..
"$B_CMAKE" --build . --parallel
echo "Build completed successfully"
