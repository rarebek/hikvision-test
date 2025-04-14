cmake_minimum_required(VERSION 3.10)
project(hikvision_stream_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Hikvision libraries
set(HIKVISION_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hikvision_sdk)
set(HIKVISION_INCLUDE_DIR ${HIKVISION_SDK_DIR})
set(HIKVISION_LIB_DIR ${HIKVISION_SDK_DIR}/lib)

# Create logs directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/logs)

# Basic example
add_executable(stream_viewer main.cpp)
target_include_directories(stream_viewer PRIVATE ${HIKVISION_INCLUDE_DIR})
target_link_directories(stream_viewer PRIVATE ${HIKVISION_LIB_DIR})
target_link_libraries(stream_viewer HCISUPStream HCISUPSS)

# Set RPATH for the executable so it can find the shared libraries
set_target_properties(stream_viewer PROPERTIES
    INSTALL_RPATH "${HIKVISION_LIB_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Advanced example that saves to file
add_executable(stream_saver main_save_to_file.cpp)
target_include_directories(stream_saver PRIVATE ${HIKVISION_INCLUDE_DIR})
target_link_directories(stream_saver PRIVATE ${HIKVISION_LIB_DIR})
target_link_libraries(stream_saver HCISUPStream)

# Set RPATH for the executable so it can find the shared libraries
set_target_properties(stream_saver PROPERTIES
    INSTALL_RPATH "${HIKVISION_LIB_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Device info utility
add_executable(get_device_info get_device_info.cpp)
target_include_directories(get_device_info PRIVATE ${HIKVISION_INCLUDE_DIR})
target_link_directories(get_device_info PRIVATE ${HIKVISION_LIB_DIR})
target_link_libraries(get_device_info HCISUPStream HCISUPSS)

# Set RPATH for the executable so it can find the shared libraries
set_target_properties(get_device_info PROPERTIES
    INSTALL_RPATH "${HIKVISION_LIB_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Install targets
install(TARGETS stream_viewer stream_saver get_device_info
    RUNTIME DESTINATION bin
)

# Update the README with build instructions
message(STATUS "Build with: mkdir -p build && cd build && cmake .. && make")
message(STATUS "Run with: ./stream_viewer <port>, ./stream_saver <port>, or ./get_device_info <port>") 