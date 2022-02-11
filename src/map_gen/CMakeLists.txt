add_subdirectory(osm)
add_subdirectory(census)
add_subdirectory(roads)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/PopulationMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/TemporalPopulationMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/OSMMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GeospatialGridMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMHandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/OSMTagGeometryHandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/GridMapOSMBuildingsHandler.cpp
        PARENT_SCOPE)