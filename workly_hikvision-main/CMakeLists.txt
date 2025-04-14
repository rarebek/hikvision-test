cmake_minimum_required(VERSION 3.10)
set(CMAKE_DEBUG_POSTFIX d)

project(hikvision_listener)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(libpq/include)
link_directories(libpq/lib)

include_directories(libsup/include)
link_directories(libisup/lib)

include_directories(src)
include_directories(src/encoders/)
#include_directories(/usr/local/include)
#link_directories(lib)
#link_directories(/usr/local/lib)
#link_directories(/usr/bin)
#set(PROJECT_LINK_LIBS -ljpeg -lpq libstdc++.a stdc++fs.a)

#NOTE: 
#libcrypto.so and libssl.so that came with SDK
#caused Segmentation fault. So, I've decided to use systems instead of
set(SDK_LIBS -lcrypto -lssl -ljpeg
libHCISUPAlarm.so libHCISUPCMS.so libHCISUPSS.so
libHCISUPStream.so libHCNetUtils.so
libhpr.so libNPQos.so
libsqlite3.so libz.so
libiconv2.so libSystemTransform.so
)

file(GLOB SOURCES "src/*.cpp" "src/**/*.cpp"  "src/encoders/**/*.cpp")

add_executable(hikvision_listener ${SOURCES})
set_target_properties(hikvision_listener PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
target_link_libraries(hikvision_listener ${SDK_LIBS} "-lstdc++fs" "-lpq")
