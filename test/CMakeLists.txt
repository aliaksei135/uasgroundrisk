##################################
# GoogleTest
# Test framework
# License: BSD-3
##################################
if (NOT TARGET gtest)
	FetchContent_Declare(
			googletest
			GIT_REPOSITORY https://github.com/google/googletest.git
			GIT_TAG release-1.11.0
	)
	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
endif ()
include(GoogleTest)

##################################
# matplot++
# C++ data viz
# License: MIT
##################################
# We do not really care about the performance of this as it is only for testing
set(WITH_SYSTEM_CIMG ON)
set(CIMG_INCLUDE_DIR ${${PROJECT_NAME}_EXTERN_DIR}/CImg)
FetchContent_Declare(
	matplotplusplus
	GIT_REPOSITORY https://github.com/alandefreitas/matplotplusplus
	GIT_TAG origin/master
)
FetchContent_GetProperties(matplotplusplus)
if(NOT matplotplusplus_POPULATED)
	FetchContent_Populate(matplotplusplus)
	add_subdirectory(${matplotplusplus_SOURCE_DIR} ${matplotplusplus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

macro(ugr_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} PUBLIC gmock_main gtest_main gtest gmock ${PROJECT_NAME})
    target_include_directories(${TESTNAME} PRIVATE osmium)
    target_include_directories(${TESTNAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
    target_include_directories(${TESTNAME} PRIVATE ${CIMG_INCLUDE_DIR})
    if(NOT DEFINED ENV{CI})
        target_compile_definitions(${TESTNAME} PRIVATE UGR_PLOT_TESTS)
        target_link_libraries(${TESTNAME} PUBLIC Matplot++::matplot)
    endif()
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(
            ${TESTNAME}
            # set a working directory so your project root so that you can find test data via paths relative to the project root
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

ugr_add_test(QueryTests OSMQueryTests.cpp)
ugr_add_test(ResponseTests OSMResponseTests.cpp)
ugr_add_test(GradientDescentTests GradientDescentTests.cpp)
ugr_add_test(ProjectionTests ProjectionTests.cpp)
ugr_add_test(PopulationMapTests PopulationMapTests.cpp)
ugr_add_test(TemporalPopulationMapTests TemporalPopulationMapTests.cpp)
ugr_add_test(GridMapOSMConstructionTests GridMapOSMConstructionTests.cpp)
ugr_add_test(AircraftModelTests AircraftDescentModelTests.cpp)
ugr_add_test(RiskMapTests RiskMapTests.cpp)
ugr_add_test(TemporalRiskMapTests TemporalRiskMapTests.cpp)
ugr_add_test(VectorOpsTests VectorOpsTests.cpp)
ugr_add_test(DataFittingTests DataFittingTests.cpp)
ugr_add_test(DataIngestTests DataIngestTests.cpp)
ugr_add_test(WeatherMapTests WeatherMapTests.cpp)
ugr_add_test(BuildingHeightMapTests BuildingHeightMapTests.cpp)
ugr_add_test(UtilTests UtilTests.cpp)
