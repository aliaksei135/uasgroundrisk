add_subdirectory(aircraft)
add_subdirectory(weather)
add_subdirectory(obstacles)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/RiskMap.cpp
        PARENT_SCOPE)