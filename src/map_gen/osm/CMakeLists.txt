add_subdirectory(builder)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/OSMOverpassQuery.cpp
        ${CMAKE_CURRENT_LIST_DIR}/OverpassExceptions.h
        PARENT_SCOPE)