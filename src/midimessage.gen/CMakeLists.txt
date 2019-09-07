cmake_minimum_required(VERSION 3.0)

set(C74_MIN_API_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/min-api)
include(${C74_MIN_API_DIR}/script/min-pretarget.cmake)

# path fix

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../externals")

#############################################################
# MAX EXTERNAL
#############################################################


include_directories(
        "${C74_INCLUDES}"
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/midimessage/include
)


set( SOURCE_FILES
        ${PROJECT_NAME}.cpp
        )


add_library(
        ${PROJECT_NAME}
        MODULE
        ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME} PUBLIC midimessage)

include(${C74_MIN_API_DIR}/script/min-posttarget.cmake)


#############################################################
# UNIT TEST
#############################################################

include(${C74_MIN_API_DIR}/test/min-object-unittest.cmake)