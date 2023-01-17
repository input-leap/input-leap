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

if (BUILD_MSWINDOWS)
    file(GLOB mswin_headers "MSWindows*.h" "ImmuneKeysReader.h" "synwinhk.h")
    file(GLOB mswin_sources "MSWindows*.cpp" "ImmuneKeysReader.cpp")
endif()
if (BUILD_CARBON)
    file(GLOB carbon_headers "OSX*.h" "IOSX*.h")
    file(GLOB carbon_sources "OSX*.cpp" "IOSX*.cpp" "OSX*.m" "OSX*.mm")
endif()
if (BUILD_XWINDOWS)
    file(GLOB xwin_headers "XWindows*.h")
    file(GLOB xwin_sources "XWindows*.cpp")
endif()

# CMake does not understand that .mm files should not go together with .cpp files into an Unity
# build that is compiled in C++ mode
set_source_files_properties(
    "OSXDragView.mm"
    "OSXPasteboardPeeker.mm"
    "OSXMediaKeySupport.mm"
    "OSXScreen.mm"
    "OSXMediaKeySimulator.mm"
    "OSXDragSimulator.mm"
    "OSXScreenSaverUtil.mm"
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE)

list(APPEND sources ${mswin_sources} ${carbon_sources} ${xwin_sources})
list(APPEND headers ${mswin_headers} ${carbon_headers} ${xwin_headers})

if(INPUTLEAP_ADD_HEADERS)
    list(APPEND sources ${headers})
endif()

if (APPLE)
    list(APPEND inc
        /System/Library/Frameworks
    )
endif()

include_directories(${inc})
add_library(platform STATIC ${sources})
target_link_libraries(platform client ${libs})

if (UNIX)
    target_link_libraries(platform io net ipc synlib client ${libs})
endif()

if (APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    target_link_libraries(platform ${COCOA_LIBRARY})
endif()
