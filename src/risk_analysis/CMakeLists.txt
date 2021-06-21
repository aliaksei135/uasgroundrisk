add_subdirectory(aircraft)
add_subdirectory(weather)

set(UGR_SOURCES
        ${UGR_SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/RiskMap.h
        ${CMAKE_CURRENT_LIST_DIR}/RiskMap.cpp
        ${CMAKE_CURRENT_LIST_DIR}/RiskEnums.h
        PARENT_SCOPE)