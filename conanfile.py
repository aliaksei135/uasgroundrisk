from conans import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake
from conan.tools.layout import cmake_layout


class ugrConan(ConanFile):
    name = "uasgroundrisk"
    version = '0.1'
    author = "Aliaksei Pilko <A.Pilko@soton.ac.uk>"
    settings = "os", "compiler", "arch", "build_type"
    license = "Proprietary"
    generators = "cmake_find_package_multi"
    build_policy = 'missing'

    exports_sources = "CMakeLists.txt", "src/*"

    def requirements(self):
        self.requires("eigen/[>3.3.9]")
        self.requires("boost/1.81.0")
        # We are careful to ONLY use the stable C API for GEOS, so can stay up to date
        self.requires("geos/3.11.1")
        # self.requires("libtiff/4.2.0")
        self.requires("shapelib/[>=1.5.0]")
        self.requires("proj/9.1.1")
        # self.requires("libcurl/[>=7.79.0]")
        # self.requires("zlib/[>=1.2.13]")
        self.requires("openssl/3.1.0")
        self.requires("cpr/1.10.1")
        self.requires("expat/2.5.0")
        # self.requires("bzip2/1.0.8")
        self.requires("fast-cpp-csv-parser/cci.20211104")
        self.requires("rapidjson/cci.20220822")
        self.requires("spdlog/[>=1.10.0]")

    def imports(self):
        self.copy('*.dll', dst='bin', src='bin')  # Copies all dll files from packages bin folder to my "bin" folder
        self.copy('*.dylib*', dst='bin',
                  src='lib')  # Copies all dylib files from packages lib folder to my "bin" folder
        self.copy('*.so*', dst='bin', src='lib')  # Copies all so files from packages lib folder to my "bin" folder

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_DOC"] = "ON"
        tc.variables["CMAKE_FIND_PACKAGE_PREFER_CONFIG"] = "ON"
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ['uasgroundrisk']
