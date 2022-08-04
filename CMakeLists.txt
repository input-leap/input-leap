# Barrier -- mouse and keyboard sharing utility
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

cmake_minimum_required (VERSION 3.4)
project(InputLeap C CXX)

option(INPUTLEAP_BUILD_GUI "Build the GUI" ON)
option(INPUTLEAP_BUILD_INSTALLER "Build the installer" ON)
option(INPUTLEAP_BUILD_TESTS "Build the tests" ON)
option(INPUTLEAP_USE_EXTERNAL_GTEST "Use external installation of Google Test framework" OFF)

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

    check_include_file_cxx (istream HAVE_ISTREAM)
    check_include_file_cxx (ostream HAVE_OSTREAM)
    check_include_file_cxx (sstream HAVE_SSTREAM)

    check_include_files (inttypes.h HAVE_INTTYPES_H)
    check_include_files (locale.h HAVE_LOCALE_H)
    check_include_files (memory.h HAVE_MEMORY_H)
    check_include_files (stdlib.h HAVE_STDLIB_H)
    check_include_files (strings.h HAVE_STRINGS_H)
    check_include_files (string.h HAVE_STRING_H)
    check_include_files (sys/select.h HAVE_SYS_SELECT_H)
    check_include_files (sys/socket.h HAVE_SYS_SOCKET_H)
    check_include_files (sys/stat.h HAVE_SYS_STAT_H)
    check_include_files (sys/time.h HAVE_SYS_TIME_H)
    check_include_files (sys/utsname.h HAVE_SYS_UTSNAME_H)
    check_include_files (unistd.h HAVE_UNISTD_H)
    check_include_files (wchar.h HAVE_WCHAR_H)

    check_function_exists (getpwuid_r HAVE_GETPWUID_R)
    check_function_exists (gmtime_r HAVE_GMTIME_R)
    check_function_exists (nanosleep HAVE_NANOSLEEP)
    check_function_exists (poll HAVE_POLL)
    check_function_exists (sigwait HAVE_POSIX_SIGWAIT)
    check_function_exists (strftime HAVE_STRFTIME)
    check_function_exists (inet_aton HAVE_INET_ATON)

    # For some reason, the check_function_exists macro doesn't detect
    # the inet_aton on some pure Unix platforms (e.g. sunos5). So we
    # need to do a more detailed check and also include some extra libs.
    if (NOT HAVE_INET_ATON)
        set (CMAKE_REQUIRED_LIBRARIES nsl)

        check_c_source_compiles (
            "#include <arpa/inet.h>\n int main() { inet_aton (0, 0); }"
            HAVE_INET_ATON_ADV)

        set (CMAKE_REQUIRED_LIBRARIES)

        if (HAVE_INET_ATON_ADV)
            # Override the previous fail.
            set (HAVE_INET_ATON 1)

            # Assume that both nsl and socket will be needed,
            # it seems safe to add socket on the back of nsl,
            # since socket only ever needed when nsl is needed.
            list (APPEND libs nsl socket)
        endif()

    endif()

    check_type_size (char SIZEOF_CHAR)
    check_type_size (int SIZEOF_INT)
    check_type_size (long SIZEOF_LONG)
    check_type_size (short SIZEOF_SHORT)

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

        pkg_check_modules (XLIB REQUIRED x11 xext xrandr xinerama xtst xi)
        pkg_check_modules (ICE REQUIRED ice sm)
        include_directories (${XLIB_INCLUDE_DIRS} ${ICE_INCLUDE_DIRS})

        # Old cmake doesn't populate LINK_LIBRARIES.
        # Use the "normal" libraries but this won't pick up non-standard
        # library directories paths.
        # Fixed in cmake 3.12, released July 2018 (commit 92ac721a44)
        if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
            list (APPEND libs ${XLIB_LIBRARIES} ${ICE_LIBRARIES})
        else()
            list (APPEND libs ${XLIB_LINK_LIBRARIES} ${ICE_LINK_LIBRARIES})
        endif()

        set (HAVE_X11 1)

        check_include_files ("dns_sd.h" HAVE_DNSSD)
        if(INPUTLEAP_BUILD_GUI AND NOT HAVE_DNSSD)
            message (FATAL_ERROR "Missing header: dns_sd.h")
        endif()
    endif()

    # For config.h, set some static values; it may be a good idea to make
    # these values dynamic for non-standard UNIX compilers.
    set (ACCEPT_TYPE_ARG3 socklen_t)
    set (HAVE_CXX_BOOL 1)
    set (HAVE_CXX_CASTS 1)
    set (HAVE_CXX_EXCEPTIONS 1)
    set (HAVE_CXX_MUTABLE 1)
    set (HAVE_CXX_STDLIB 1)
    set (SELECT_TYPE_ARG1 int)
    set (SELECT_TYPE_ARG234 " (fd_set *)")
    set (SELECT_TYPE_ARG5 " (struct timeval *)")
    set (STDC_HEADERS 1)
    set (TIME_WITH_SYS_TIME 1)
    set (HAVE_SOCKLEN_T 1)

    add_definitions (-DSYSAPI_UNIX=1)

    if (APPLE)
        add_definitions (-DWINAPI_CARBON=1 -D_THREAD_SAFE)
    else()
        add_definitions (-DWINAPI_XWINDOWS=1)
    endif()

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D _BIND_TO_CURRENT_VCLIBS_VERSION=1")
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
endif()

# For config.h, save the results based on a template (config.h.in).
configure_file(res/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/lib/config.h)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/gulrak-filesystem/include")

#
# OpenSSL
#
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set (OPENSSL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/ext/openssl/windows)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (OPENSSL_ROOT "${OPENSSL_ROOT}/x64")
    else()
        set (OPENSSL_ROOT "${OPENSSL_ROOT}/x86")
    endif()

    include_directories (BEFORE SYSTEM ${OPENSSL_ROOT}/include)
    set (OPENSSL_LIBS
        ${OPENSSL_ROOT}/lib/libeay32.lib
        ${OPENSSL_ROOT}/lib/ssleay32.lib
    )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_program(APT_PROGRAM "apt")
    find_program(BREW_PROGRAM "brew")
    find_program(PORT_PROGRAM "port")

    if (IS_DIRECTORY /opt/procursus AND APT_PROGRAM)
        # procursus/apt
        set (OPENSSL_ROOT /opt/procursus)

        include_directories (BEFORE SYSTEM ${OPENSSL_ROOT}/include)

        set (OPENSSL_LIBS
            ${OPENSSL_ROOT}/lib/libssl.a
            ${OPENSSL_ROOT}/lib/libcrypto.a
        )
    elseif (IS_DIRECTORY /opt/local AND PORT_PROGRAM)
        # macports
        set (OPENSSL_ROOT /opt/local)

        set (OPENSSL_LIBS
            ${OPENSSL_ROOT}/lib/libssl.a
            ${OPENSSL_ROOT}/lib/libcrypto.a
            z
        )
    elseif (IS_DIRECTORY /usr/local/opt/openssl AND BREW_PROGRAM)
        # brew
        set (OPENSSL_ROOT /usr/local/opt/openssl)

        include_directories (BEFORE SYSTEM ${OPENSSL_ROOT}/include)

        set (OPENSSL_LIBS
            ${OPENSSL_ROOT}/lib/libssl.a
            ${OPENSSL_ROOT}/lib/libcrypto.a
        )
    elseif (IS_DIRECTORY /opt/homebrew/opt/openssl AND BREW_PROGRAM)
        # brew
        set (OPENSSL_ROOT /opt/homebrew/opt/openssl)

        include_directories (BEFORE SYSTEM ${OPENSSL_ROOT}/include)

        set (OPENSSL_LIBS
            ${OPENSSL_ROOT}/lib/libssl.a
            ${OPENSSL_ROOT}/lib/libcrypto.a
        )
    endif()

elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (OPENSSL_LIBS ssl crypto)
else()
    find_library (lib_ssl ssl)
    find_library (lib_crypto crypto)
    if (NOT lib_ssl)
        message(FATAL_ERROR "openssl library not found")
    elseif (NOT lib_crypto)
        message(FATAL_ERROR "crypto library not found")
    endif()
    set (OPENSSL_LIBS ${lib_ssl} ${lib_crypto})
endif()

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
    set(INPUTLEAP_BUNDLE_APP_DIR ${INPUTLEAP_BUNDLE_DIR}/Barrier.app)
    set(INPUTLEAP_BUNDLE_BINARY_DIR ${INPUTLEAP_BUNDLE_APP_DIR}/Contents/MacOS)

    configure_files(${INPUTLEAP_BUNDLE_SOURCE_DIR} ${INPUTLEAP_BUNDLE_DIR})

    add_custom_target(Barrier_MacOS ALL
                      bash build_dist.sh
                      DEPENDS barrier barriers barrierc
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
    install(FILES doc/barrierc.1 doc/barriers.1 DESTINATION share/man/man1)

    configure_file(res/barrier.appdata.xml.in ${CMAKE_CURRENT_BINARY_DIR}/res/barrier.appdata.xml)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/res/barrier.appdata.xml DESTINATION share/metainfo)

    configure_files (${CMAKE_CURRENT_SOURCE_DIR}/dist/rpm ${CMAKE_BINARY_DIR}/rpm)
    install(FILES res/barrier.svg DESTINATION share/icons/hicolor/scalable/apps)
    if("${VERSION_MAJOR}" STREQUAL "2")
        install(FILES res/barrier2.desktop DESTINATION share/applications)
    else()
        install(FILES res/barrier.desktop DESTINATION share/applications)
    endif()
endif()

else()
    message (STATUS "NOT configuring the installer")
endif()

enable_testing()

add_subdirectory (src)
