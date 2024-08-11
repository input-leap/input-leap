####~~~~~~~~~~~~~~~~~~~~~~git_version_from_tag~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#git_version_from_tag(OUTPUT <var-name> [MAJOR <value>] [MINOR <value>] [PATCH <value>] [TWEAK <value> ])
#This Function will set the variable <var_name> to semantic version value based on the last git tag
#This Requires a tag in the format of vX.Y.Z in order to construct a proper verson
## REQUIRED ARGUMENTS
# OUTPUT <value> - The name of the variable the version will be written into
## OPTIONAL ARGUMENTS
# MAJOR <value> - the MAJOR argument sets the fallback major to use if its unable to be detected [Default: 0]
# MINOR <value> - the MINOR argument sets the fallback minor to use if its unable to be detected [Default: 0]
# PATCH <value> - the PATCH argument sets the fallback patch to use if its unable to be detected [Default: 0]
# TWEAK <value> - the TWEAK argument sets the fallback tweak to use if its unable to be detected [Default: 0]
#Optional MAJOR, MINOR, PATCH should be set when calling they will be used if git can not be found or tag can not be processed.For this reason the MAJOR, MINOR and PATCH should should be synced with semantic tag in git
#The Tweak is auto generated based on the number of commits since the last tag
function(git_version_from_tag)
    set(options)
    set(oneValueArgs
        OUTPUT # The Variable to write into
        MAJOR # Fallback Version Major
        MINOR # Fallback Version Minor
        PATCH # Fallback Version Patch
        TWEAK # Fallback Version Patch
    )
    set(multiValueArgs)
    cmake_parse_arguments(m "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(m_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${m_UNPARSED_ARGUMENTS}")
    endif()

    if("${m_OUTPUT}" STREQUAL "")
        message(FATAL_ERROR "No OUTPUT set")
    endif()

    if(NOT m_MAJOR)
        set(m_MAJOR 0)
    endif()

    if(NOT m_MINOR)
        set(m_MINOR 0)
    endif()

    if(NOT m_PATCH)
        set(m_PATCH 0)
    endif()

    if(NOT m_TWEAK)
        set(m_TWEAK 0)
    endif()

    set(VERSION_MAJOR ${m_MAJOR})
    set(VERSION_MINOR ${m_MINOR})
    set(VERSION_PATCH ${m_PATCH})
    set(VERSION_TWEAK ${m_TWEAK})

    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
        find_package(Git)
        if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --long --match v* --always
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            OUTPUT_VARIABLE GITREV
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(FIND ${GITREV} "v" isRev)
            if(NOT isRev EQUAL -1)
                string(REGEX MATCH [0-9]+ MAJOR ${GITREV})
                string(REGEX MATCH \\.[0-9]+ MINOR ${GITREV})
                string(REPLACE "." "" MINOR "${MINOR}")
                string(REGEX MATCH [0-9]+\- PATCH ${GITREV})
                string(REPLACE "-" "" PATCH "${PATCH}")
                string(REGEX MATCH \-[0-9]+\- TWEAK ${GITREV})
                string(REPLACE "-" "" TWEAK "${TWEAK}")
                set(VERSION_MAJOR ${MAJOR})
                set(VERSION_MINOR ${MINOR})
                set(VERSION_PATCH ${PATCH})
                set(VERSION_TWEAK ${TWEAK})
            elseif(NOT ${GITREV} STREQUAL "")
                message(STATUS "Unable to process tag")
            endif()
        endif()
    endif()
    set(${m_OUTPUT} "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}" PARENT_SCOPE)

    string(TIMESTAMP INPUTLEAP_BUILD_DATE "%Y%m%d" UTC)
    set(INPUTLEAP_BUILD_DATE ${INPUTLEAP_BUILD_DATE} PARENT_SCOPE)

    string(TIMESTAMP INPUTLEAP_BUILD_TIME "%H%M" UTC)
    set(INPUTLEAP_BUILD_TIME ${INPUTLEAP_BUILD_TIME} PARENT_SCOPE)

    string(TIMESTAMP INPUTLEAP_BUILD_DATE_ISO "%Y-%m-%d" UTC)
    set(INPUTLEAP_BUILD_DATE_ISO ${INPUTLEAP_BUILD_DATE_ISO} PARENT_SCOPE)

    add_definitions(-DINPUTLEAP_BUILD_DATE="${INPUTLEAP_BUILD_DATE}")

    if (DEFINED ENV{BUILD_NUMBER})
        set(INPUTLEAP_BUILD_NUMBER $ENV{BUILD_NUMBER} PARENT_SCOPE)
    else()
        set(INPUTLEAP_BUILD_NUMBER 1 PARENT_SCOPE)
    endif()
    add_definitions(-DINPUTLEAP_BUILD_NUMBER=${INPUTLEAP_BUILD_NUMBER})
endfunction()
