cmake_minimum_required(VERSION 3.16)
project(uasgroundrisk
    DESCRIPTION "Ground Risk calculation for Unmanned Aerial Systems"
    VERSION 0.1
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PYTHON_VERSION 3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(${PROJECT_NAME}_EXTERN_DIR ${PROJECT_SOURCE_DIR}/extern)
Include(FetchContent)
include(GNUInstallDirs)
include_guard(GLOBAL)

# Verbose Builds
# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

### Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
    message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif ()

# Cache builds
find_program(CCACHE ccache)
if (CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
endif ()


option(BUILD_SHARED_LIBS "Build libraries as shared libraries" ON)
option(UGR_BUILD_TESTS "Set to ON to build uasgroundrisk tests" OFF)
option(UGR_BUILD_DOCS "Set to ON to build uasgroundrisk documentation" OFF)

###########################################################
# Static code analysis
###########################################################
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(ENABLE_CPPCHECK OFF)
else ()
    set(ENABLE_CPPCHECK OFF)
endif ()
if (MSVC)
    set(ENABLE_CLANG_TIDY OFF)
else ()
    set(ENABLE_CLANG_TIDY ON)
endif ()

if (ENABLE_CPPCHECK)
    find_program(CPPCHECK cppcheck)
    if (CPPCHECK)
    set(CMAKE_CXX_CPPCHECK
        ${CPPCHECK}
        --suppressions-list=${CMAKE_CURRENT_SOURCE_DIR}/.cppcheck-suppressions
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --cppcheck-build-dir=${CMAKE_BINARY_DIR}/cppcheck
        --enable=all
        --std=c++17
        --inconclusive)
    else ()
    message(WARNING "cppcheck requested but executable not found")
    endif ()
endif ()

if (ENABLE_CLANG_TIDY)
    find_program(CLANGTIDY clang-tidy)
    if (CLANGTIDY)
    set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY})
    else ()
    message(WARNING "clang-tidy requested but executable not found")
    endif ()
endif ()

##############################################################
# Packaging + Linking
##############################################################

set(UGR_SOURCES
    ${UGR_SOURCES})

add_subdirectory(src)

if (MSVC)
    set(BUILD_SHARED_LIBS OFF)
endif ()

add_library(${PROJECT_NAME} ${UGR_SOURCES})

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
add_library(${PROJECT_NAME}::ugr ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>)

target_sources(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/gridmap/GridMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/gridmap/Iterators.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/gridmap/TypeDefs.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/PopulationMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/TemporalPopulationMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/OSMMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/GeospatialGridMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/OSMTag.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/OSMOverpassQuery.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/OSMOverpassQueryBuilder.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/handlers/GridMapOSMHandler.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/handlers/OSMTagGeometryHandler.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/IncrementalRiskMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/RiskMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/RiskEnums.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/aircraft/AircraftDescentModel.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/aircraft/AircraftStateModel.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/aircraft/AircraftEnums.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/weather/WeatherMap.h
    ${PROJECT_SOURCE_DIR}/include/uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h
    )


if (MSVC)
    # Prevent Windows.h overwriting min and max with macros
    # Keep Math defines such as M_PI
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX _USE_MATH_DEFINES)
    target_compile_options(${PROJECT_NAME} PRIVATE /permissive-)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -pedantic-errors)
endif ()

if (NOT (${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
    if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
    elseif (GCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fno-math-errno -fno-signaling-nans -fno-trapping-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fno-math-errno -fno-signaling-nans -fno-trapping-math")
    endif ()
endif ()

# Data bundling
if (DEFINED ENV{UGR_DATA_DIR})
    set(UGR_DATA_DIR $ENV{UGR_DATA_DIR} CACHE PATH "uasgroundrisk static data directory")
elseif (DEFINED UGR_DATA_DIR)
    # Keep existing CMake variable if already defined
    set(UGR_DATA_DIR ${UGR_DATA_DIR} CACHE PATH "uasgroundrisk static data directory")
else()
    set(UGR_DATA_DIR ${PROJECT_BINARY_DIR}/data CACHE PATH "uasgroundrisk static data directory")
endif ()

add_custom_target(ugr_bundled_data
    ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/extern/data ${UGR_DATA_DIR})
add_dependencies(${PROJECT_NAME} ugr_bundled_data)
# @formatter:off
target_compile_definitions(${PROJECT_NAME} PUBLIC UGR_DATA_DIR="${UGR_DATA_DIR}")
# @formatter:on
if (DEFINED $ENV{UGR_OMP_NESTED})
    if ($ENV{UGR_OMP_NESTED} STREQUAL "ON")
    message("UGR OMP Nested loops enabled")
    target_compile_definitions(${PROJECT_NAME} PUBLIC UGR_OMP_NESTED)
    endif ()
endif ()

##############################################################
# Dependencies
##############################################################
list(APPEND CMAKE_MODULE_PATH ${PROJECT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR})

# Find dependencies using Conan v2
find_package(Eigen3 REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC Eigen3::Eigen)

find_package(cpr REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)

find_package(PROJ REQUIRED CONFIG)

# Get Data dir in conan package
string(TOUPPER ${CMAKE_BUILD_TYPE} ${PROJECT_NAME}_UPPER_BUILD_TYPE)
set(PROJ_DATA "${proj_PACKAGE_FOLDER_${${PROJECT_NAME}_UPPER_BUILD_TYPE}}/res")

# check if PROJ_DATA is empty
if (NOT DEFINED PROJ_DATA OR PROJ_DATA STREQUAL "")
    # Hacky way to get the data dir from the conan package
    message(WARNING "Could not find PROJ data dir for build type ${CMAKE_BUILD_TYPE} using proj_PACKAGE_FOLDER_${${PROJECT_NAME}_UPPER_BUILD_TYPE} config target variable")
    set(PROJ_DATA "${PROJ_INCLUDE_DIR_${${PROJECT_NAME}_UPPER_BUILD_TYPE}}/../res/")
else ()
    message(STATUS "Found PROJ data dir: ${PROJ_DATA}")
endif ()

# Check that proj.db actually exists in PROJ_DATA
if (NOT EXISTS "${PROJ_DATA}/proj.db")
    message(FATAL_ERROR "Could not find proj.db in assumed PROJ_DATA dir: ${PROJ_DATA}")
endif ()

add_custom_target(UGR_proj_data
    ${CMAKE_COMMAND} -E copy_directory ${PROJ_DATA} ${PROJECT_BINARY_DIR}/bin)
add_dependencies(${PROJECT_NAME} UGR_proj_data)
# @formatter:off
# Set this to a compile def that is used when PROJ contexts are created
target_compile_definitions(${PROJECT_NAME} PUBLIC PROJ_DATA_PATH="${PROJ_DATA}")
# @formatter:on
target_link_libraries(${PROJECT_NAME} PUBLIC PROJ::proj)

find_package(GEOS REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC GEOS::geos_c)

find_package(shapelib REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC shapelib::shp)

find_package(Boost REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)

find_package(fast-cpp-csv-parser REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC fast-cpp-csv-parser::fast-cpp-csv-parser)

find_package(RapidJSON REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC rapidjson)

find_package(spdlog REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog)

find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
endif ()

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

##################################
# Osmium
# OSM data libraries
# License: BSLv1
##################################
FetchContent_Declare(
    osmium
    GIT_REPOSITORY https://github.com/osmcode/libosmium.git
    GIT_TAG v2.18.0)
FetchContent_GetProperties(osmium)
if (NOT osmium_POPULATED)
    FetchContent_Populate(osmium)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${osmium_SOURCE_DIR}/cmake) # use osmium cmake scripts to find its deps
    target_include_directories(${PROJECT_NAME} PUBLIC ${osmium_SOURCE_DIR}/include)
endif ()

find_package(EXPAT REQUIRED CONFIG)
target_link_libraries(${PROJECT_NAME} PRIVATE expat::expat)

find_package(BZip2 CONFIG)
if (BZIP2_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE BZip2::BZip2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UGR_IO_COMPRESSION_BZ2)
endif ()

##############################################################
# Testing
##############################################################

if (UGR_BUILD_TESTS OR BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()

#############################################################
# Install
#############################################################
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")


#############################################################
# Docs
#############################################################

if (UGR_BUILD_DOCS)
    find_package(Doxygen CONFIG)
    if (DOXYGEN_FOUND)
    SET(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
    SET(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Building Doxygen documentation"
        VERBATIM)
    else (DOXYGEN_FOUND)
    message("No doxygen binary found on the system.")
    endif ()
endif ()
