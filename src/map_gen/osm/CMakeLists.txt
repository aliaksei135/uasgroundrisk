add_subdirectory(builder)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/OSMOverpassQuery.h
        ${CMAKE_CURRENT_LIST_DIR}/OSMOverpassQuery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/OverpassExceptions.h
        ${CMAKE_CURRENT_LIST_DIR}/DefaultNodeLocationsForWaysHandler.h
        PARENT_SCOPE)