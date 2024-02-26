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

    # Check if user preference is set for MacPorts
    if [ "$LEAP__SYS_PKGMGR" = "macports" ]; then
        printf "Using Macports as specified by user\n"

        check_dir_exists '/opt/local/lib/cmake/Qt5' 'qt5-qtbase port'

        export INPUTLEAP_BUILD_MACPORTS=1
        export CMAKE_PREFIX_PATH="/opt/local/lib/cmake/Qt5:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/local/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/local/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/local/libexec/qt5/lib/pkgconfig:$PKG_CONFIG_PATH"

    # Check if user preference is set for Homebrew
    elif [ "$LEAP__SYS_PKGMGR" = "homebrew" ]; then
        printf "Using Homebrew as specified by user\n"
        QT_PATH=$(brew --prefix qt@5)

        check_dir_exists "$QT_PATH" 'qt5'

        export INPUTLEAP_BUILD_BREW=1
        export CMAKE_PREFIX_PATH="/opt/procursus:$QT_PATH:$CMAKE_PREFIX_PATH"
        export LD_LIBRARY_PATH="/opt/procursus/lib:$LD_LIBRARY_PATH"
        export CPATH="/opt/procursus/include:$CPATH"
        export PKG_CONFIG_PATH="/opt/procursus/lib/pkgconfig:$PKG_CONFIG_PATH"

    # Default behavior if no user preference is set
    else
        if command -v port; then
            printf "Detected Macports\n"
            # Set environment variables for MacPorts...
        elif command -v brew; then
            printf "Detected Homebrew\n"
            # Set environment variables for Homebrew...
        else
            printf "Neither Homebrew nor Macports is installed. Can't get dependency paths\n"
            exit 1
        fi
    fi

    export INPUTLEAP_BUILD_ENV=1
    printf "done\n"
fi
