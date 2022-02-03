#include "uasgroundrisk/risk_analysis/obstacles/ObstacleMap.h"

#include "uasgroundrisk/map_gen/osm/handlers/GridMapOSMBuildingsHandler.h"

void ugr::risk::ObstacleMap::addOSMHandler(osmium::handler::Handler& handler)
{
    // const auto pHandler = std::unique_ptr<osmium::handler::Handler>(&handler);
    // // handlers.push_back(&handler);
}

void ugr::risk::ObstacleMap::addBuildingHeights()
{
    addOSMLayer("Building Height", {mapping::osm::OSMTag("building")});
}

void ugr::risk::ObstacleMap::eval()
{
    mapping::GridMapOSMBuildingsHandler handler(this);
    OSMMap::eval(handler);
}
