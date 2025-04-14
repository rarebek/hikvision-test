cmake_minimum_required(VERSION 3.10)
project(hikvision_device_info)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the Hikvision SDK include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/hikvision_sdk/incEn
)

# Add the Hikvision SDK library directories
link_directories(
    ${CMAKE_SOURCE_DIR}/hikvision_sdk/lib
)

# Create executable
add_executable(hikvision_device_info main.cpp)

# Link against Hikvision libraries
target_link_libraries(hikvision_device_info
    ${CMAKE_SOURCE_DIR}/hikvision_sdk/lib/libHCISUPCMS.so
    ${CMAKE_SOURCE_DIR}/hikvision_sdk/lib/libHCISUPSS.so
    ${CMAKE_SOURCE_DIR}/hikvision_sdk/lib/libHCISUPStream.so
    pthread
)

# Create logs directory
add_custom_command(
    TARGET hikvision_device_info
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
) 