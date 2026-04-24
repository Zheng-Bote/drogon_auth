cmake_minimum_required(VERSION 3.31)

project(drogon_auth
    VERSION 0.1.0
    DESCRIPTION "A high-performance C++23 Drogon-based Authentication Microservice"
    HOMEPAGE_URL "https://github.com/zheng-bote/drogon_auth"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Conan dependencies
find_package(drogon REQUIRED)
find_package(libsodium REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/config_util.cpp
    src/auth_srv.cpp
    src/auth_ctrl.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    libsodium::libsodium
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)

# libs.txt Generate (timing-safe via GENERATE)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/libs.txt"
     CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")

# Tests
enable_testing()
add_executable(test_auth tests/test_auth.cpp src/config_util.cpp src/auth_srv.cpp)
target_include_directories(test_auth PRIVATE include)
target_link_libraries(test_auth PRIVATE Catch2::Catch2WithMain libsodium::libsodium jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto Drogon::Drogon)
add_test(NAME test_auth COMMAND test_auth)
