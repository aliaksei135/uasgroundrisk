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
    
    def build_requirements(self):
        self.build_requires("doxygen/[>=1.9.1]")
    
    def requirements(self):
        self.requires("eigen/[>3.3.9]")
        self.requires("boost/1.78.0")
        # We are careful to ONLY use the stable C API for GEOS, so can stay up to date
        self.requires("geos/3.10.2")
        self.requires("libtiff/4.2.0")
        self.requires("shapelib/[>=1.5.0]")
        self.requires("proj/8.2.1")
        self.requires("libcurl/7.79.1")
        self.requires("cpr/[>=1.6.2]")
        self.requires("expat/2.4.1")
        self.requires("bzip2/1.0.8")
        self.requires("fast-cpp-csv-parser/cci.20200830")
        self.requires("rapidjson/cci.20211112")
        self.requires("spdlog/[>=1.10.0]")

    
    def imports(self):
        self.copy('*.dll', dst='bin', src='bin') # Copies all dll files from packages bin folder to my "bin" folder
        self.copy('*.dylib*', dst='bin', src='lib') # Copies all dylib files from packages lib folder to my "bin" folder
        self.copy('*.so*', dst='bin', src='lib')# Copies all so files from packages lib folder to my "bin" folder
        
        
    def layout(self):
        cmake_layout(self)


    def generate(self):
        self.deps_cpp_info["doxygen"].set_property("skip_deps_file", True)
        
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
