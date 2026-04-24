from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps

class DrogonAuthConan(ConanFile):
    name = "drogon_auth"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("drogon/1.9.4")
        self.requires("libsodium/1.0.19")
        self.requires("jwt-cpp/0.7.0")
        self.requires("openssl/3.2.1")
        self.requires("catch2/3.5.2")

    def layout(self):
        cmake_layout(self)
