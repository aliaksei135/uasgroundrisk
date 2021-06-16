add_subdirectory(map_gen)
add_subdirectory(pathfinding)
add_subdirectory(risk_analysis)
add_subdirectory(utils)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/library.cpp
        PARENT_SCOPE)