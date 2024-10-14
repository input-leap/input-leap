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

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

if (WIN32)
    file(GLOB arch_headers "win32/*.h")
    file(GLOB arch_sources "win32/*.cpp")
elseif (UNIX)
    file(GLOB arch_headers "unix/*.h")
    file(GLOB arch_sources "unix/*.cpp")
endif()

list(APPEND headers ${arch_headers})
list(APPEND sources ${arch_sources})

if(INPUTLEAP_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

add_library(common STATIC ${sources})

include(FetchContent)
FetchContent_Declare(cmake_git_version_tracking
        GIT_REPOSITORY https://github.com/andrew-hardin/cmake-git-version-tracking.git
        GIT_TAG 9b5fc5088b4089ff2adc20d607976b9923e3d737
)
FetchContent_MakeAvailable(cmake_git_version_tracking)
target_link_libraries(common cmake_git_version_tracking)

if (HAVE_GETPWUID_R)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_GETPWUID_R=1")
endif()

