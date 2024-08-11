# InputLeap -- mouse and keyboard sharing utility
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

#Write our Verison info to a file
configure_file(Version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp @ONLY)

set(common_HEADERS
    common.h
    DataDirectories.h
    Version.h
)

set(common_SOURCES
    DataDirectories_static.cpp
    DataDirectoriesCommon.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp
)

if (WIN32)
    list(APPEND common_HEADERS
        win32/encoding_utilities.h
        win32/winapi.h
    )

    list(APPEND common_SOURCES
        win32/encoding_utilities.cpp
        win32/DataDirectories.cpp
    )
elseif (UNIX)
    list(APPEND common_SOURCES
        unix/DataDirectories.cpp
    )
endif()

if(INPUTLEAP_ADD_HEADERS)
    list(APPEND sources ${common_HEADERS})
endif()

add_library(common STATIC ${common_SOURCES})

if (HAVE_GETPWUID_R)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_GETPWUID_R=1")
endif()


