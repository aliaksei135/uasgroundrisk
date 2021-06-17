cmake_minimum_required(VERSION 3.16)
project(uasgroundrisk
        DESCRIPTION "Ground Risk calculation for Unmanned Aerial Systems"
        VERSION 0.1
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(PYTHON_VERSION 3)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
Include(FetchContent)
include_guard(GLOBAL)

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

###########################################################
# Static code analysis
###########################################################
set(ENABLE_CPPCHECK OFF)
set(ENABLE_CLANG_TIDY ON)

if (ENABLE_CPPCHECK)
    find_program(CPPCHECK cppcheck)
    if (CPPCHECK)
        set(CMAKE_CXX_CPPCHECK
                ${CPPCHECK}
                --suppressions-list=${CMAKE_CURRENT_SOURCE_DIR}/.cppcheck-suppressions
                --suppress=missingIncludeSystem
                --enable=all
                --inconclusive
                -i${CMAKE_CURRENT_BINARY_DIR}/_deps) #Do not check subprojects
    else ()
        message(SEND_ERROR "cppcheck requested but executable not found")
    endif ()
endif ()

if (ENABLE_CLANG_TIDY)
    find_program(CLANGTIDY clang-tidy)
    if (CLANGTIDY)
        set(CMAKE_CXX_CLANG_TIDY ${CLANGTIDY})
    else ()
        message(SEND_ERROR "clang-tidy requested but executable not found")
    endif ()
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

##############################################################
# Packaging + Linking
##############################################################

set(UGR_SOURCES
        ${UGR_SOURCES})

add_subdirectory(src)
include_directories(include)

add_library(${PROJECT_NAME} ${UGR_SOURCES})

##############################################################
# Dependencies
##############################################################
# Eigen required for lin alg
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
# Prevent ...eigen3/... in all includes
include_directories(/usr/include/eigen3)

# OpenSSL for cpr https requests
find_package(OpenSSL REQUIRED)

find_package(GEOS REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${GEOS_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${GEOS_LIBRARY})

#find_package(PROJ REQUIRED)
#target_include_directories(${PROJECT_NAME} PRIVATE ${PROJ_INCLUDE_DIR})
#target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJ_LIBRARY})

# Googletest
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)

##################################
# PROJ
# Geographical CRS transformations
# License: MIT-Like
##################################
set(ENV{PROJ_NETWORK} ON)
set(BUILD_TESTING OFF)
FetchContent_Declare(
        proj
        GIT_REPOSITORY https://github.com/OSGeo/PROJ.git
        GIT_TAG 8.0.0
)
FetchContent_GetProperties(proj)
if (NOT proj_POPULATED)
    FetchContent_Populate(proj)
    set(ENABLE_CURL ON)
    add_subdirectory(${proj_SOURCE_DIR})
    include_directories(${proj_SOURCE_DIR}/include)
    add_dependencies(proj generate_proj_db) # Download and configure PROJ transformation grid data
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC proj)

##################################
# GEOS
# Geometry ops
# License: LGPL
##################################

#FetchContent_Declare(
#        geos
#        GIT_REPOSITORY https://git.osgeo.org/gitea/geos/geos.git
#        GIT_TAG 3.5.2 # This should not be changed as GEOS C++ API is unstable, osmium also breaks >3.5
#)
#FetchContent_GetProperties(geos)
#if (NOT geos_POPULATED)
#    FetchContent_Populate(geos)
#    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${geos_SOURCE_DIR}/cmake) # use osmium cmake scripts to find its deps
#    add_subdirectory(${geos_SOURCE_DIR})
#    include_directories(${geos_SOURCE_DIR}/include)
#endif()
#
#execute_process(COMMAND 'geos-config --cflags'
#                OUTPUT_VARIABLE GEOS_COMPILE_FLAGS)
#execute_process(COMMAND 'geos-config --ldflags'
#                OUTPUT_VARIABLE GEOS_LINK_FLAGS)
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GEOS_COMPILE_FLAGS}")
#SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GEOS_LINK_FLAGS} -lgeos_c")

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
    include_directories(${osmium_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME} PRIVATE expat) # Link expat (built by osmium) for xml io
endif ()
find_package(BZip2 REQUIRED)
if (BZIP2_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE BZip2::BZip2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UGR_IO_COMPRESSION_BZ2)
endif ()

##################################
# cpr
# Like python requests
# License: MIT
##################################
if (WIN32)
    set(CPR_FORCE_WINSSL_BACKEND ON)
else ()
    set(CPR_FORCE_OPENSSL_BACKEND ON)
endif ()

# https://github.com/whoshuu/cpr/issues/445
set(CURL_CA_FALLBACK ON)
set(USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)

set(CPR_BUILD_TESTS OFF)

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/whoshuu/cpr.git
        GIT_TAG 1.6.0
)
FetchContent_MakeAvailable(cpr)
target_link_libraries(${PROJECT_NAME} PRIVATE cpr::cpr)
include_directories(${cpr_SOURCE_DIR}/include)

##################################
# grid_map
# Grid Map and Octomap interface
# License: BSD-3
##################################
set(GRID_MAP_VERSION 1.6.4)
set(CATKIN_ENABLE_TESTING OFF)

FetchContent_Declare(
        grid_map
        GIT_REPOSITORY https://github.com/anybotics/grid_map.git
        GIT_TAG ${GRID_MAP_VERSION}
)
FetchContent_GetProperties(grid_map)
if (NOT grid_map_POPULATED)
    FetchContent_Populate(grid_map)
    add_subdirectory(${grid_map_SOURCE_DIR}/grid_map_core)
    include_directories(${grid_map_SOURCE_DIR}/grid_map_core/include)
endif ()
target_link_libraries(${PROJECT_NAME} PUBLIC grid_map_core)


##############################################################
# Testing
##############################################################

set(${PROJECT_NAME}_BUILD_TESTS ON)

if (${PROJECT_NAME}_BUILD_TESTS)
    ##################################
    # GoogleTest
    # Test framework
    # License: BSD-3
    ##################################
    if (NOT TARGET gtest)
        FetchContent_Declare(
                googletest
                GIT_REPOSITORY https://github.com/google/googletest.git
                GIT_TAG v1.10.x
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif ()

    ##################################
    # matplotlib_cpp
    # C++ wrapper around matplotlib for plotting of tests
    # License: BSD-3
    ##################################
    # We do not really care about the performance of this as it is only for testing
    FetchContent_Declare(
            matplotlib_cpp
            GIT_REPOSITORY https://github.com/lava/matplotlib-cpp.git
            GIT_TAG 61501081ea32549df1a02dca26cb4edbe0b6a890
    )
    FetchContent_GetProperties(matplotlib_cpp)
    if (NOT matplotlib_cpp_POPULATED)
        FetchContent_Populate(matplotlib_cpp)
        include_directories(${matplotlib_cpp_SOURCE_DIR})
    endif ()

    enable_testing()
    add_subdirectory(test)
endif ()