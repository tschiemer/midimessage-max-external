cmake_minimum_required(VERSION 3.0)

string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")


#if (${CMAKE_GENERATOR} MATCHES "Xcode")
#    if (${XCODE_VERSION} VERSION_LESS 9)
#        message(STATUS "Xcode 9 or higher is required. Please install from the Mac App Store.")
#        return ()
#    endif ()
#endif ()


# Fetch the correct version of the min-api
#message(STATUS "Updating Git Submodules")
#execute_process(
#        COMMAND				git submodule update --init --recursive
#        WORKING_DIRECTORY	"${CMAKE_CURRENT_SOURCE_DIR}"
#)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/min-api)


# Misc setup and subroutines
include(${C74_MIN_API_DIR}/script/min-package.cmake)


# Add unit tests for the API
#enable_testing()
add_subdirectory(${C74_MIN_API_DIR})


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/midimessage)




# Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/min-lib)
endif ()


# Generate a project for every folder in the "source/projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/${project_dir})
    endif ()
endforeach ()