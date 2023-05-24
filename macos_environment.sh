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

    printf "Modifying environment for InputLeap build...\n"

    if command -v port; then
        printf "Detected Macports\n"

        check_dir_exists '/opt/local/lib/cmake/Qt6' 'qt6-qtbase port'

        export INPUTLEAP_BUILD_MACPORTS=1
        export CMAKE_PREFIX_PATH="/opt/local/lib/cmake/Qt6:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/local/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/local/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/local/libexec/qt6/lib/pkgconfig:$PKG_CONFIG_PATH"

    elif command -v brew; then
        printf "Detected Homebrew\n"
        QT_PATH=$(brew --prefix qt)

        check_dir_exists "$QT_PATH" 'qt'

        export INPUTLEAP_BUILD_BREW=1
        export CMAKE_PREFIX_PATH="/opt/procursus:$QT_PATH:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/procursus/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/procursus/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/procursus/lib/pkgconfig:$PKG_CONFIG_PATH"
    else
        printf "Neither Homebrew nor Macports is installed. Can't get dependency paths\n"
        exit 1
    fi

    export INPUTLEAP_BUILD_ENV=1

    printf "done\n"
fi
