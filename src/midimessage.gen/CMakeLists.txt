cmake_minimum_required(VERSION 3.0)

# Setup script pre-target
include(${CMAKE_SOURCE_DIR}/scripts/pretarget.cmake)


############# Target Setup

include_directories(
        "${C74_INCLUDES}"
        "${MIDIMESSAGE_INCLUDES}"
)

set( SOURCE_FILES ${PROJECT_NAME}.cpp )

add_library(${PROJECT_NAME} MODULE ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC midimessage)


############# / Target Setup


# Post-target finish script
include(${CMAKE_SOURCE_DIR}/scripts/posttarget.cmake)

