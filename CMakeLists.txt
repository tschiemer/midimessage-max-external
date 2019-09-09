cmake_minimum_required(VERSION 3.0)
project(midimessage_max_external)

# Configuration
set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/min-api)
set(MIDIMESSAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/midimessage)
set(ENABLE_TESTING false)

#set(BUNDLE_IDENTIFIER "minimessage.max")
#set(AUTHOR_DOMAIN "se.filou")
#set(COPYRIGHT_STRING "2019 Philip Tschiemer")

# (MAX MIN-API) ?
string(REGEX REPLACE "(.*)/" "" THIS_PACKAGE_NAME "${CMAKE_CURRENT_SOURCE_DIR}")


# (MAX MIN-API)
if (${CMAKE_GENERATOR} MATCHES "Xcode")
    if (${XCODE_VERSION} VERSION_LESS 9)
        message(STATUS "Xcode 9 or higher is required. Please install from the Mac App Store.")
        return ()
    endif ()
endif ()


# (MAX MIN-API) Fetch the correct version submodules
message(STATUS "Updating Git Submodules")
execute_process(
        COMMAND				git submodule update --init --recursive
        WORKING_DIRECTORY	"${CMAKE_CURRENT_SOURCE_DIR}"
)



# (MAX MIN-API) Misc setup and subroutines
include(${C74_MIN_API_DIR}/script/min-package.cmake)

# (MAX MIN-API) Add unit tests for the API
if(${ENABLE_TESTING})
    enable_testing()
endif()
add_subdirectory(${C74_MIN_API_DIR})


# Add midimessage library and includes
add_subdirectory(${MIDIMESSAGE_DIR} EXCLUDE_FROM_ALL)
set(MIDIMESSAGE_INCLUDES ${MIDIMESSAGE_DIR}/include)


# (MAX MIN-API) Add the Lib, if it exists
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/min-lib/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/min-lib)
endif ()


# (MAX MIN-API) Generate a project for every folder in the "source/projects" folder
SUBDIRLIST(PROJECT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src/)
foreach (project_dir ${PROJECT_DIRS})
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${project_dir}/CMakeLists.txt")
        message("Generating: ${project_dir}")
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/${project_dir})
    endif ()
endforeach ()