cmake_minimum_required(VERSION 3.10)
project(tws)

set(CMAKE_CXX_STANDARD 20)

# Include directories for project sources and IB TWS API headers
include_directories(
        "${CMAKE_SOURCE_DIR}/src"
        "/home/m41k1/IBJts/source/cppclient/client"
        "/home/m41k1/IBJts/source/cppclient/lib"
)

# Build the executable from main.cpp and TwsApi.cpp
add_executable(tws
        main.cpp
        src/TwsApi.cpp
        src/DecimalFunctions.cpp
)


target_link_libraries(tws
        "/home/m41k1/IBJts/source/cppclient/client/libTwsSocketClient.so"
        "/home/m41k1/IBJts/source/cppclient/lib/libbid.so"
        pthread
)

# Set the runtime rpath so that the executable can locate the shared libraries
set_target_properties(tws PROPERTIES
        BUILD_RPATH "/home/m41k1/IBJts/source/cppclient/client:/home/m41k1/IBJts/source/cppclient/lib"
        INSTALL_RPATH "/home/m41k1/IBJts/source/cppclient/client:/home/m41k1/IBJts/source/cppclient/lib"
)
