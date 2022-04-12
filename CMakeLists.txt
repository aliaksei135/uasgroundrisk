cmake_minimum_required(VERSION 3.16)
project(uasgroundrisk
		DESCRIPTION "Ground Risk calculation for Unmanned Aerial Systems"
		VERSION 0.1
		LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CXX_STANDARD_REQUIRED ON)
set(PYTHON_VERSION 3)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(${PROJECT_NAME}_EXTERN_DIR ${PROJECT_SOURCE_DIR}/extern)
Include(FetchContent)
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

option(UGR_BUILD_TESTS "Build tests for uasgroundrisk" OFF)

###########################################################
# Static code analysis
###########################################################
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(ENABLE_CPPCHECK OFF)
else()
	set(ENABLE_CPPCHECK OFF)
endif()
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
	set(${PROJECT_NAME}_BUILD_SHARED OFF)
else ()
	set(${PROJECT_NAME}_BUILD_SHARED ON)
endif ()

if (${${PROJECT_NAME}_BUILD_SHARED})
	add_library(${PROJECT_NAME} SHARED ${UGR_SOURCES})
else ()
	add_library(${PROJECT_NAME} STATIC ${UGR_SOURCES})
endif ()
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
endif ()

if(NOT (${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
	if(MSVC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
	elseif(GCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fno-math-errno -fno-signaling-nans -fno-trapping-math")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fno-math-errno -fno-signaling-nans -fno-trapping-math")
	endif()
endif()

# Data bundling
if(NOT ENV{UGR_DATA_DIR})
	set(UGR_DATA_DIR ${PROJECT_BINARY_DIR}/data CACHE PATH "uasgroundrisk static data directory")
else()
	set(UGR_DATA_DIR ENV{UGR_DATA_DIR} CACHE PATH "uasgroundrisk static data directory")
endif()

add_custom_target(ugr_bundled_data
	${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/extern/data ${UGR_DATA_DIR})
add_dependencies(${PROJECT_NAME} ugr_bundled_data)
target_compile_definitions(${PROJECT_NAME} PUBLIC UGR_DATA_DIR="${UGR_DATA_DIR}")

##############################################################
# Dependencies
##############################################################
list(APPEND CMAKE_MODULE_PATH ${PROJECT_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_BINARY_DIR})

if (NOT EXISTS "${PROJECT_BINARY_DIR}/conan.cmake")
	message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
	file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
			"${CMAKE_BINARY_DIR}/conan.cmake"
			EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
			TLS_VERIFY ON)
endif ()
include(${CMAKE_BINARY_DIR}/conan.cmake)
conan_cmake_run(CONANFILE conanfile.py
		BASIC_SETUP
		BUILD missing)

# Don't build tests for deps
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

# Eigen required for lin alg
find_package(Eigen3 CONFIG REQUIRED)
set(EIGEN3_FOUND TRUE) # This doesn't get set by the conan generated find macro

# OpenSSL for cpr https requests
find_package(OpenSSL CONFIG REQUIRED)

find_package(cpr CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)

find_package(proj CONFIG REQUIRED)
# Get Data dir in conan package
string(TOUPPER ${CMAKE_BUILD_TYPE} ${PROJECT_NAME}_UPPER_BUILD_TYPE)
set(PROJ_DATA ${PROJ_proj_RES_DIRS_${${PROJECT_NAME}_UPPER_BUILD_TYPE}})
# Set this to a compile def that is used when PROJ contexts are created
target_compile_definitions(${PROJECT_NAME} PUBLIC PROJ_DATA_PATH="${PROJ_DATA}")
target_link_libraries(${PROJECT_NAME} PUBLIC PROJ::PROJ)

find_package(geos CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC GEOS::geos_c)

find_package(shapelib CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC shapelib::shp)

#find_package(boost CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)
#target_link_libraries(${PROJECT_NAME} PUBLIC boost::)

find_package(fast-cpp-csv-parser CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC fast-cpp-csv-parser::fast-cpp-csv-parser)

find_package(RapidJSON CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC RapidJSON::RapidJSON)

find_package(OpenMP REQUIRED)
target_link_directories(${PROJECT_NAME} PUBLIC OpenMP::OpenMP_CXX)
if(OPENMP_FOUND)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

find_package(Threads REQUIRED)

##################################
# Osmium
# OSM data libraries
# License: BSLv1
##################################
FetchContent_Declare(
		osmium
		GIT_REPOSITORY https://github.com/osmcode/libosmium.git
		GIT_TAG v2.16.0)
FetchContent_GetProperties(osmium)
if (NOT osmium_POPULATED)
	FetchContent_Populate(osmium)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${osmium_SOURCE_DIR}/cmake) # use osmium cmake scripts to find its deps
	target_include_directories(${PROJECT_NAME} PUBLIC ${osmium_SOURCE_DIR}/include)
endif ()

find_package(EXPAT CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE expat::expat)

find_package(BZip2 CONFIG)
if (BZIP2_FOUND)
	target_link_libraries(${PROJECT_NAME} PRIVATE BZip2::BZip2)
	target_compile_definitions(${PROJECT_NAME} PRIVATE UGR_IO_COMPRESSION_BZ2)
endif ()

##############################################################
# Testing
##############################################################

if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME OR UGR_BUILD_TESTs) AND BUILD_TESTING)
	enable_testing()
	add_subdirectory(test)
endif ()

#############################################################
# Docs
#############################################################

if (BUILD_DOC)
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		SET(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in)
		SET(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

		add_custom_target(doc ALL
				COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
				COMMENT "Buidling Doxygen documentation"
				VERBATIM)
	else (DOXYGEN_FOUND)
		message("No doxygen binary found on the system.")
		SET(${BUILD_DOC} OFF)
	endif ()
endif ()