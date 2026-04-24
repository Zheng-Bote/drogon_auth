from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout, CMakeDeps

class DrogonAuthConan(ConanFile):
    name = "drogon_auth"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
        "drogon/*:with_postgres": True,
        "drogon/*:with_sqlite3": True,
        "libpq/*:with_gssapi": False
    }

    def requirements(self):
        self.requires("drogon/[>=1.9.12]")
        self.requires("trantor/[>=1.5.26]")
        self.requires("c-ares/[>=1.25.0]")
        self.requires("jsoncpp/[>=1.9.5]")
        self.requires("util-linux-libuuid/[>=2.39.2]")
        self.requires("boost/[>=1.83.0]")
        self.requires("bzip2/[>=1.0.8]")
        self.requires("libbacktrace/cci.20210118")
        self.requires("jwt-cpp/[>=0.7.2]")
        self.requires("openssl/[>=3.6.2]")
        self.requires("zlib/[>=1.3.1]")
        self.requires("argon2/20190702")
        self.requires("catch2/[>=3.14.0]")
        self.requires("libcurl/[>=8.19.0]")

    def layout(self):
        cmake_layout(self)
