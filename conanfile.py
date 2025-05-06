from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout


class UasgroundriskConan(ConanFile):
    name = "uasgroundrisk"
    version = "0.1"
    author = "Aliaksei Pilko <A.Pilko@soton.ac.uk>"
    license = "Proprietary"
    settings = "os", "compiler", "arch", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "src/*"

    def requirements(self):
        self.requires("eigen/[>3.3.9]")
        self.requires("boost/1.81.0")
        self.requires("geos/3.13.0")
        self.requires("shapelib/1.5.0")
        self.requires("proj/9.5.0")
        self.requires("cpr/1.10.5")
        self.requires("expat/2.7.1")
        self.requires("fast-cpp-csv-parser/cci.20211104")
        self.requires("rapidjson/cci.20220822")
        self.requires("spdlog/[>=1.10.0]")

    # def layout(self):
    #     cmake_layout(self)

    # def build(self):
    #     cmake = CMake(self)
    #     cmake.configure(
    #         variables={"BUILD_DOC": "ON", "CMAKE_FIND_PACKAGE_PREFER_CONFIG": "ON"}
    #     )
    #     cmake.build()

    # def package(self):
    #     cmake = CMake(self)
    #     cmake.install()

    # def package_info(self):
    #     self.cpp_info.libs = ["uasgroundrisk"]
