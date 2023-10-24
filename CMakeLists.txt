# InputLeap -- mouse and keyboard sharing utility
# Copyright (C) 2018 Debauchee Open Source Group
# Copyright (C) 2012-2016 Symless Ltd.
# Copyright (C) 2009 Nick Bolton
#
# This package is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# found in the file LICENSE that should have accompanied this file.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.21)
project(InputLeap C CXX)

option(INPUTLEAP_BUILD_GUI "Build the GUI" ON)
option(INPUTLEAP_BUILD_INSTALLER "Build the installer" ON)
option(INPUTLEAP_BUILD_TESTS "Build the tests" ON)
option(INPUTLEAP_USE_EXTERNAL_GTEST "Use external installation of Google Test framework" OFF)
option(INPUTLEAP_BUILD_X11 "Build with XWindows support" ON)
option(INPUTLEAP_BUILD_LIBEI "Build with libei support" OFF)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions (-DNDEBUG)
endif()

include (cmake/Version.cmake)
include (cmake/Package.cmake)

# TODO: Find out why we need these, and remove them
if (COMMAND cmake_policy)
    cmake_policy (SET CMP0003 NEW)
    cmake_policy (SET CMP0005 NEW)
endif()

# Add headers to source list
if (${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
    set(INPUTLEAP_ADD_HEADERS FALSE)
else()
    set(INPUTLEAP_ADD_HEADERS TRUE)
endif()

set (libs)

if (UNIX)
    if (NOT APPLE)
        set (CMAKE_POSITION_INDEPENDENT_CODE TRUE)
    endif()

    # For config.h, detect the libraries, functions, etc.
    include (CheckIncludeFiles)
    include (CheckLibraryExists)
    include (CheckFunctionExists)
    include (CheckTypeSize)
    include (CheckIncludeFileCXX)
    include (CheckSymbolExists)
    include (CheckCSourceCompiles)
    include (FindPkgConfig)

    check_include_files (sys/socket.h HAVE_SYS_SOCKET_H)
    check_include_files (sys/utsname.h HAVE_SYS_UTSNAME_H)

    check_function_exists (getpwuid_r HAVE_GETPWUID_R)

    # pthread is used on both Linux and Mac
    set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set (THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package (Threads REQUIRED)
    list (APPEND libs Threads::Threads)

    # curl is used on both Linux and Mac
    find_package (CURL)
    if (CURL_FOUND)
        include_directories(${CURL_INCLUDE_DIRS})
        list (APPEND libs ${CURL_LIBRARIES})

    else()
        message (FATAL_ERROR "Missing library: curl")
    endif()

    if (APPLE)
        set (CMAKE_CXX_FLAGS "--sysroot ${CMAKE_OSX_SYSROOT} ${CMAKE_CXX_FLAGS} -DGTEST_USE_OWN_TR1_TUPLE=1")

        find_library (lib_ScreenSaver ScreenSaver)
        find_library (lib_IOKit IOKit)
        find_library (lib_ApplicationServices ApplicationServices)
        find_library (lib_Foundation Foundation)
        find_library (lib_Carbon Carbon)

        list (APPEND libs
            ${lib_ScreenSaver}
            ${lib_IOKit}
            ${lib_ApplicationServices}
            ${lib_Foundation}
            ${lib_Carbon}
        )

    else() # not-apple
        if (NOT ${PKG_CONFIG_FOUND})
            message (FATAL_ERROR "pkg-config not found, required for dependencies")
        endif ()

        # FreeBSD uses /usr/local for anything not part of base
        # Also package avahi-libdns puts dns_sd.h a bit deeper
        if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
            set (CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};/usr/local/include;/usr/local/include/avahi-compat-libdns_sd")
            set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L/usr/local/lib")
            include_directories("/usr/local/include" "/usr/local/include/avahi-compat-libdns_sd")
            link_directories("/usr/local/lib")
        endif()

        if (${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
            set (CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};/usr/X11R6/include;/usr/local/include;/usr/local/include/avahi-compat-libdns_sd")
            set (CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -L/usr/local/lib -L/usr/X11R6/lib")
            include_directories("/usr/local/include" "/usr/X11R6/include" "/usr/local/include/avahi-compat-libdns_sd")
            link_directories("/usr/local/lib")
            link_directories("/usr/X11R6/lib")
        endif()

        if(INPUTLEAP_BUILD_GUI AND ${PKG_CONFIG_FOUND})
            pkg_check_modules (AVAHI_COMPAT REQUIRED avahi-compat-libdns_sd)
            include_directories (BEFORE SYSTEM ${AVAHI_COMPAT_INCLUDE_DIRS})
            set (CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${AVAHI_COMPAT_INCLUDE_DIRS}")
        endif ()

        if (INPUTLEAP_BUILD_X11)
            pkg_check_modules (XLIB REQUIRED x11 xext xrandr xinerama xtst xi)
            pkg_check_modules (ICE REQUIRED ice sm)
            include_directories (${XLIB_INCLUDE_DIRS} ${ICE_INCLUDE_DIRS})

            list(APPEND libs ${XLIB_LINK_LIBRARIES} ${ICE_LINK_LIBRARIES})

            set (HAVE_X11 1)
        endif()

        if (INPUTLEAP_BUILD_LIBEI)
            pkg_check_modules(LIBEI REQUIRED "libei-1.0 >= 0.99.1")
            pkg_check_modules(LIBXKBCOMMON REQUIRED xkbcommon)
            pkg_check_modules(GLIB2 REQUIRED glib-2.0 gio-2.0)
            pkg_check_modules(LIBPORTAL REQUIRED libportal)
            include_directories(${LIBEI_INCLUDE_DIRS} ${LIBXKBCOMMON_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${LIBPORTAL_INCLUDE_DIRS})
            list(APPEND libs ${LIBEI_LINK_LIBRARIES} ${LIBXKBCOMMON_LINK_LIBRARIES} ${GLIB2_LINK_LIBRARIES} ${LIBPORTAL_LINK_LIBRARIES})

            # no libportal release has xdp_session_connect_to_eis, so let's check for that
            # and enable the hackaround in the code
            include(CMakePushCheckState)
            include(CheckCXXSourceCompiles)
            cmake_push_check_state(RESET)
            set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES};${LIBPORTAL_INCLUDE_DIRS};${GLIB2_INCLUDE_DIRS}")
            set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES};${LIBPORTAL_LINK_LIBRARIES};${GLIB2_LINK_LIBRARIES}")
            check_symbol_exists(xdp_session_connect_to_eis "libportal/portal.h" HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS)
            check_symbol_exists(xdp_input_capture_session_connect_to_eis "libportal/inputcapture.h" HAVE_LIBPORTAL_INPUTCAPTURE)
            # check_symbol_exists can’t check for enum values
            check_cxx_source_compiles("#include <libportal/portal.h>
                int main() { XdpOutputType out = XDP_OUTPUT_NONE; }
            " HAVE_LIBPORTAL_OUTPUT_NONE)
            cmake_pop_check_state()

            # Flatpak bits
            install(FILES "dist/flatpak/input-leap-flatpak"
                    DESTINATION bin
                    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        endif()

        check_include_files ("dns_sd.h" HAVE_DNSSD)
        if(INPUTLEAP_BUILD_GUI AND NOT HAVE_DNSSD)
            message (FATAL_ERROR "Missing header: dns_sd.h")
        endif()
    endif()

    # For config.h, set some static values; it may be a good idea to make
    # these values dynamic for non-standard UNIX compilers.
    set (ACCEPT_TYPE_ARG3 socklen_t)
    set (SELECT_TYPE_ARG1 int)
    set (SELECT_TYPE_ARG234 " (fd_set *)")
    set (SELECT_TYPE_ARG5 " (struct timeval *)")

    add_definitions (-DSYSAPI_UNIX=1)

    if (APPLE)
        add_definitions (-DWINAPI_CARBON=1 -D_THREAD_SAFE)
        set (BUILD_CARBON 1)
    else()
        if(INPUTLEAP_BUILD_X11)
            add_definitions (-DWINAPI_XWINDOWS=1)
            set (BUILD_XWINDOWS 1)
        endif()
        if(INPUTLEAP_BUILD_LIBEI)
            add_definitions(-DWINAPI_LIBEI=1)
            set(BUILD_LIBEI 1)
        endif()
        if(NOT INPUTLEAP_BUILD_X11 AND NOT INPUTLEAP_BUILD_LIBEI)
            message(FATAL_ERROR "One of X11 or libei is required")
        endif()
    endif()

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /Zi")
    set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /O2 /Ob2")

    list (APPEND libs Wtsapi32 Userenv Wininet comsuppw Shlwapi)

    add_definitions (
        /DSYSAPI_WIN32=1
        /DWINAPI_MSWINDOWS=1
        /D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING=1  # tr1 is used from gtest and gmock
        /DWIN32
        /D_WINDOWS
        /D_CRT_SECURE_NO_WARNINGS
        /DINPUTLEAP_VERSION=\"${INPUTLEAP_VERSION}\"
        /D_XKEYCHECK_H
    )
    set (BUILD_MSWINDOWS 1)
endif()

# For config.h, save the results based on a template (config.h.in).
configure_file(res/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/lib/config.h)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/gulrak-filesystem/include")

#
# OpenSSL
#
# Find Static Libs on mac OS and windows
if (APPLE OR WIN32)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()
find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS SSL Crypto)
#
# Configure_file... but for directories, recursively.
#
macro (configure_files srcDir destDir)
    message (STATUS "Configuring directory ${destDir}")
    make_directory (${destDir})

    file (GLOB_RECURSE sourceFiles RELATIVE ${srcDir} ${srcDir}/*)
    file (GLOB_RECURSE templateFiles LIST_DIRECTORIES false RELATIVE ${srcDir} ${srcDir}/*.in)
    list (REMOVE_ITEM sourceFiles ${templateFiles})

    foreach (sourceFile ${sourceFiles})
        set (sourceFilePath ${srcDir}/${sourceFile})
        if (IS_DIRECTORY ${sourceFilePath})
            message (STATUS "Copying directory ${sourceFile}")
            make_directory (${destDir}/${sourceFile})
        else()
            message (STATUS "Copying file ${sourceFile}")
            configure_file (${sourceFilePath} ${destDir}/${sourceFile} COPYONLY)
        endif()
    endforeach (sourceFile)

    foreach (templateFile ${templateFiles})
        set (sourceTemplateFilePath ${srcDir}/${templateFile})
                string (REGEX REPLACE "\.in$" "" templateFile ${templateFile})
        message (STATUS "Configuring file ${templateFile}")
        configure_file (${sourceTemplateFilePath} ${destDir}/${templateFile} @ONLY)
    endforeach (templateFile)
endmacro (configure_files)

if (${INPUTLEAP_BUILD_INSTALLER})
#
# macOS app Bundle
#
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (CMAKE_INSTALL_RPATH "@loader_path/../Libraries;@loader_path/../Frameworks")
    set(INPUTLEAP_BUNDLE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/dist/macos/bundle)
    set(INPUTLEAP_BUNDLE_DIR ${CMAKE_BINARY_DIR}/bundle)
    set(INPUTLEAP_BUNDLE_APP_DIR ${INPUTLEAP_BUNDLE_DIR}/InputLeap.app)
    set(INPUTLEAP_BUNDLE_BINARY_DIR ${INPUTLEAP_BUNDLE_APP_DIR}/Contents/MacOS)

    configure_files(${INPUTLEAP_BUNDLE_SOURCE_DIR} ${INPUTLEAP_BUNDLE_DIR})

    add_custom_target(InputLeap_MacOS ALL
                      bash build_dist.sh
                      DEPENDS input-leap input-leaps input-leapc
                      WORKING_DIRECTORY ${INPUTLEAP_BUNDLE_DIR})
endif()

#
# Windows installer
#
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(INPUTLEAP_WIX_VERSION "${INPUTLEAP_VERSION_MAJOR}.${INPUTLEAP_VERSION_MINOR}.${INPUTLEAP_VERSION_PATCH}")
        message (STATUS "Configuring the wix installer")
        configure_files (${CMAKE_CURRENT_SOURCE_DIR}/dist/wix ${CMAKE_BINARY_DIR}/installer-wix)
        message (STATUS "Configuring the inno installer")
        configure_files (${CMAKE_CURRENT_SOURCE_DIR}/dist/inno ${CMAKE_BINARY_DIR}/installer-inno)
endif()

#
# Linux installation
#
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    install(FILES doc/input-leapc.1 doc/input-leaps.1 DESTINATION share/man/man1)

    configure_file(res/input-leap.appdata.xml.in
                   ${CMAKE_CURRENT_BINARY_DIR}/res/input-leap.appdata.xml)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/res/input-leap.appdata.xml DESTINATION share/metainfo)

    configure_files (${CMAKE_CURRENT_SOURCE_DIR}/dist/rpm ${CMAKE_BINARY_DIR}/rpm)
    install(FILES res/input-leap.svg DESTINATION share/icons/hicolor/scalable/apps)
    if("${VERSION_MAJOR}" STREQUAL "2")
        install(FILES res/input-leap2.desktop DESTINATION share/applications)
    else()
        install(FILES res/input-leap.desktop DESTINATION share/applications)
    endif()
endif()

else()
    message (STATUS "NOT configuring the installer")
endif()

enable_testing()

add_subdirectory (src)
