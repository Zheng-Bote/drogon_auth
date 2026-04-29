cmake_minimum_required(VERSION 3.31)

# conan install . --output-folder=build --build=missing -s build_type=Debug
# cmake --preset conan-debug
# cmake --build --preset conan-debug --parallel $(nproc)

project(drogon_auth
    VERSION 0.4.0
    DESCRIPTION "A high-performance C++23 Drogon-based Authentication Microservice"
    HOMEPAGE_URL "https://github.com/zheng-bote/drogon_auth"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Configure (für rz_config.hpp) ---
set(EXECUTABLE_NAME "${PROJECT_NAME}")
set(PROG_LONGNAME "Drogon Authentication Microservice")
set(PROG_CREATED "2026")
set(PROG_COPYRIGHT "ZHENG Robert")
set(PROG_LICENSE "Apache-2.0") 
set(PROG_AUTHOR "ZHENG Bote")
set(PROG_ORGANIZATION_NAME "ZHENG Robert")
set(PROG_ORGANIZATION_DOMAIN "net.hase-zheng")
# create rz_config.hpp
add_subdirectory(configure)


# --- Dependencies via FetchContent ---
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE ALWAYS)
include(FetchContent)

FetchContent_Declare(
  gh_update_checker
  GIT_REPOSITORY https://github.com/Zheng-Bote/gh-update-checker.git
  GIT_TAG        v1.0.9
)

FetchContent_MakeAvailable(gh_update_checker)

# Find Conan dependencies
find_package(Drogon REQUIRED)
find_package(jwt-cpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Catch2 REQUIRED)
find_package(libsodium REQUIRED)
find_package(CURL REQUIRED)

# Force libpq to link against GSSAPI (because Conan built it with GSSAPI)
if(TARGET libpq::libpq)
    set_property(TARGET libpq::libpq APPEND PROPERTY INTERFACE_LINK_LIBRARIES "-lgssapi_krb5" "-lkrb5" "-lk5crypto" "-lcom_err")
endif()


add_executable(${PROJECT_NAME}
    src/main.cpp
    src/auth_srv.cpp
    src/auth_ctrl.cpp
    src/system_ctrl.cpp
    src/auth_middleware.cpp
    src/utils/config_utils.cpp
    src/utils/totp_utils.cpp
    src/utils/password_utils.cpp
    src/utils/seeder_utils.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
    libsodium::libsodium
    gh_update_checker::gh_update_checker
    CURL::libcurl
)

# libs.txt Generate (timing-safe via GENERATE)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_libs.txt"
     CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")

# Tests
enable_testing()
add_executable(test_auth tests/test_auth.cpp src/utils/config_utils.cpp src/utils/password_utils.cpp src/utils/totp_utils.cpp src/auth_srv.cpp)
target_include_directories(test_auth PRIVATE include)
target_link_libraries(test_auth PRIVATE Catch2::Catch2WithMain jwt-cpp::jwt-cpp OpenSSL::SSL OpenSSL::Crypto libsodium::libsodium Drogon::Drogon)
add_test(NAME test_auth COMMAND test_auth)
