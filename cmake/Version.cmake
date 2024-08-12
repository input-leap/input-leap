
set(INPUTLEAP_VERSION_MAJOR 3)
set(INPUTLEAP_VERSION_MINOR 0)
set(INPUTLEAP_VERSION_PATCH 0)
set(INPUTLEAP_VERSION_TWEAK 0)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    find_package(Git)
    if(GIT_FOUND)
        ##COMMIT
        execute_process (
            COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE INPUTLEAP_REV
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(LENGTH "${INPUTLEAP_REV}" INPUTLEAP_REVISION_LENGTH)
        if (INPUTLEAP_REVISION_LENGTH EQUAL 8 AND INPUTLEAP_REV MATCHES "^[a-f0-9]+")
            string(TOUPPER ${INPUTLEAP_REV} INPUTLEAP_REV)
            set(INPUTLEAP_REVISION "${INPUTLEAP_REV}")
        endif()
        unset(INPUTLEAP_REVISION_LENGTH)
        ##VERSION
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --long --match v* --always
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE GITREV
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(FIND ${GITREV} "v" isRev)
        if(NOT isRev EQUAL -1)
            string(REGEX MATCH [0-9]+ MAJOR ${GITREV})
            string(REGEX MATCH \\.[0-9]+ MINOR ${GITREV})
            string(REPLACE "." "" MINOR "${MINOR}")
            string(REGEX MATCH [0-9]+\- PATCH ${GITREV})
            string(REPLACE "-" "" PATCH "${PATCH}")
            string(REGEX MATCH \-[0-9]+\- TWEAK ${GITREV})
            string(REPLACE "-" "" TWEAK "${TWEAK}")
            set(INPUTLEAP_VERSION_MAJOR ${MAJOR})
            set(INPUTLEAP_VERSION_MINOR ${MINOR})
            set(INPUTLEAP_VERSION_PATCH ${PATCH})
            set(INPUTLEAP_VERSION_TWEAK ${TWEAK})
            unset(MAJOR)
            unset(MINOR)
            unset(PATCH)
            unset(TWEAK)
        elseif(NOT ${GITREV} STREQUAL "")
            message(STATUS "Unable to process tag")
        endif()
    endif()
endif()

set(INPUTLEAP_VERSION "${INPUTLEAP_VERSION_MAJOR}.${INPUTLEAP_VERSION_MINOR}.${INPUTLEAP_VERSION_PATCH}.${INPUTLEAP_VERSION_TWEAK}")

if (DEFINED ENV{BUILD_NUMBER})
    set(INPUTLEAP_BUILD_NUMBER $ENV{BUILD_NUMBER})
else()
    set(INPUTLEAP_BUILD_NUMBER 1)
endif()

string(TIMESTAMP INPUTLEAP_BUILD_DATE "%Y%m%d" UTC)
string(TIMESTAMP INPUTLEAP_BUILD_TIME "%H%M" UTC)
string(TIMESTAMP INPUTLEAP_BUILD_DATE_ISO "%Y-%m-%d" UTC)

message(STATUS "InputLeap version is ${INPUTLEAP_VERSION}, Revision: ${INPUTLEAP_REVISION}, Built on ${INPUTLEAP_BUILD_DATE} ${INPUTLEAP_BUILD_TIME}")

add_definitions(-DINPUTLEAP_REVISION="${INPUTLEAP_REVISION}")
add_definitions(-DINPUTLEAP_BUILD_DATE="${INPUTLEAP_BUILD_DATE}")
add_definitions(-DINPUTLEAP_BUILD_NUMBER=${INPUTLEAP_BUILD_NUMBER})

if(INPUTLEAP_DEVELOPER_MODE)
    add_definitions(-DINPUTLEAP_DEVELOPER_MODE=1)
endif()
