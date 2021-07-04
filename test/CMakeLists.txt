include(GoogleTest)
find_package(Python3 COMPONENTS Interpreter Development NumPy REQUIRED)
execute_process(COMMAND "pip3 install numpy matplotlib")


macro(ugr_add_test TESTNAME)
    # create an exectuable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main fuction to
    # the test executable.  Remove g_test_main if writing your own main function.
    target_link_libraries(${TESTNAME} PUBLIC gmock_main gtest_main gtest gmock Python3::NumPy Python3::Python ${PROJECT_NAME})
    include_directories(osmium)
    include_directories(${CMAKE_CURRENT_LIST_DIR})
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(
            ${TESTNAME}
            # set a working directory so your project root so that you can find test data via paths relative to the project root
            WORKING_DIRECTORY ${PROJECT_DIR}
            PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_DIR}"
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()

ugr_add_test(QueryTests OSMQueryTests.cpp)
ugr_add_test(ResponseTests OSMResponseTests.cpp)
ugr_add_test(MappingTests MappingTests.cpp)
ugr_add_test(ProjectionTests ProjectionTests.cpp)
ugr_add_test(PopulationMapTests PopulationMapTests.cpp)
ugr_add_test(GridMapOSMConstructionTests GridMapOSMConstructionTests.cpp)
ugr_add_test(AircraftModelTests AircraftDescentModelTests.cpp)
ugr_add_test(RiskMapTests RiskMapTests.cpp)
ugr_add_test(VectorOpsTests VectorOpsTests.cpp)
