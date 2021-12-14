add_subdirectory(osm)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/PopulationMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GeospatialGridMap.h
        ${CMAKE_CURRENT_LIST_DIR}/GeospatialGridMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMHandler.h
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMHandler.cpp
        PARENT_SCOPE)