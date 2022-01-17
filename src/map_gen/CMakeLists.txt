add_subdirectory(osm)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/PopulationMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GeospatialGridMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMHandler.h
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMHandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMBuildingsHandler.h
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMBuildingsHandler.cpp
        PARENT_SCOPE)