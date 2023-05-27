#!/bin/bash

# Checks if directory exists, otherwise asks to install package.
check_dir_exists() {
    local path=$1
    local package=$2

    if [ ! -d "$path" ]; then
        echo "Please install $package"
        exit 1
    fi
}

if [ -z "$INPUTLEAP_BUILD_ENV" ]; then
    check_dir_exists '/Applications/Xcode.app' 'Xcode'

    echo "Modifying environment for InputLeap build..."

    if command -v port >/dev/null; then
        echo "Detected Macports"
        QT_PATH="/opt/local/lib/cmake/Qt${QT_VERSION}"
        check_dir_exists "$QT_PATH" "qt${QT_VERSION}-qtbase port" 'https://www.macports.org/install.php'
        export INPUTLEAP_BUILD_MACPORTS=1
        export CMAKE_PREFIX_PATH="$QT_PATH:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/local/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/local/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/local/libexec/qt${QT_VERSION}/lib/pkgconfig:$PKG_CONFIG_PATH"

    elif command -v brew >/dev/null; then
        echo "Detected Homebrew"
        QT_PATH=$(brew --prefix qt)
        check_dir_exists "$QT_PATH" "qt" 'https://brew.sh'
        export INPUTLEAP_BUILD_BREW=1
        export CMAKE_PREFIX_PATH="/opt/procursus:$QT_PATH:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/procursus/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/procursus/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/procursus/lib/pkgconfig:$PKG_CONFIG_PATH"
    else
        echo "Neither Homebrew nor Macports is installed. Can't get dependency paths" >&2
        exit 1
    fi

    export INPUTLEAP_BUILD_ENV=1

    printf "done\n"
fi
