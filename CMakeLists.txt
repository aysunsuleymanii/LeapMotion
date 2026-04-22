cmake_minimum_required(VERSION 3.16)
project(LeapMotion)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LEAP_SDK_PATH ${CMAKE_SOURCE_DIR}/external/ultraleap)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(leap ${SRC_FILES})

target_include_directories(leap PRIVATE
        ${LEAP_SDK_PATH}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(leap PRIVATE
        ${LEAP_SDK_PATH}/lib/libLeapC.dylib
)

set_target_properties(leap PROPERTIES
        BUILD_RPATH   "${LEAP_SDK_PATH}/lib"
        INSTALL_RPATH "${LEAP_SDK_PATH}/lib"
)

if(APPLE)
    target_link_libraries(leap PRIVATE
            "-framework ApplicationServices"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
    )
endif()